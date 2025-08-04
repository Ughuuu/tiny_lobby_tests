#include "main.h"

// Define BasePath default constructor
BasePath::BasePath() = default;

// Implementation of BasePath
BasePath &BasePath::instance() {
    static BasePath inst;
    return inst;
}
void BasePath::set(const std::string &path) {
    std::lock_guard<std::mutex> lock(mutex_);
    base_path_ = path;
    if (!base_path_.empty() && base_path_.back() != '/' && base_path_.back() != '\\') {
        base_path_ += "/";
    }
}
std::string BasePath::get() const { return base_path_; }
std::string BasePath::file(const std::string &fname) const { return base_path_ + fname; }

#include <readerwriterqueue.h>
#include <stddef.h>
#include <uwebsockets/App.h>

#include <atomic>
#include <filesystem>
#include <thread>

#include "INIReader.h"
#include "database.h"
#include "default_config.h"
#include "game_thread.h"
#include "websocket_server.h"
#ifdef _WIN32
#include <windows.h>
#endif

std::string getUserAppDataPath(const std::string &appName = "LobbyServer") {
    std::string path;

#ifdef _WIN32
    const char *appData = std::getenv("APPDATA");  // Usually C:\Users\Username\AppData\Roaming
    if (appData)
        path = std::string(appData) + "\\" + appName + "\\";
    else
        path = ".\\" + appName + "\\";

#elif __APPLE__
    const char *home = std::getenv("HOME");
    if (home)
        path = std::string(home) + "/Library/Application Support/" + appName + "/";
    else
        path = "./" + appName + "/";

#elif __linux__
    const char *configHome = std::getenv("XDG_CONFIG_HOME");
    const char *home = std::getenv("HOME");

    if (configHome)
        path = std::string(configHome) + "/" + appName + "/";
    else if (home)
        path = std::string(home) + "/.config/" + appName + "/";
    else
        path = "./" + appName + "/";
#else
    path = "./" + appName + "/";
#endif

    return path;
}

std::string get_jwt_path() { return getUserAppDataPath("LobbyServer") + "session.jwt"; }

std::vector<std::string> get_game_data_without_game_id(const std::string &game_id) {
    std::vector<std::string> lines;
    std::ifstream games_file_check(BasePath::instance().file("games.ini"));
    if (!games_file_check.is_open()) {
        return lines;
    }
    std::string line;
    bool in_game_section = false;
    while (std::getline(games_file_check, line)) {
        if (line == "[" + game_id + "]") {
            in_game_section = true;
        } else if (in_game_section && line.starts_with("[")) {
            in_game_section = false;
        }
        if (!in_game_section) {
            lines.push_back(line);
        }
    }
    return lines;
}

std::string extract_data_from_new_game(const std::string &resp, const std::string &game_id) {
    yyjson_doc *doc = yyjson_read(resp.c_str(), resp.size(), 0);
    if (!doc) {
        return "Missing or invalid JSON data.";
    }

    yyjson_val *root = yyjson_doc_get_root(doc);
    std::string lobby_control = decode_string_or_default(root, "lobby_control", "lua");
    int send_rate = decode_int_or_default(root, "sendrate", 0);
    bool seal = decode_int_or_default(root, "seal", false);
    int tick_rate = decode_int_or_default(root, "tickrate", 0);

    yyjson_doc_free(doc);
    std::vector<std::string> lines = get_game_data_without_game_id(game_id);
    std::ofstream games_file(BasePath::instance().file("games.ini"), std::ios::out);
    if (!games_file.is_open()) {
        return "Failed to open games.ini.";
    }
    lines.push_back("[" + game_id + "]");
    lines.push_back("lobby_control=" + lobby_control);
    if (send_rate > 0) {
        lines.push_back("sendrate=" + std::to_string(send_rate));
    }
    if (seal > 0) {
        lines.push_back("seal=" + std::to_string(seal));
    }
    if (tick_rate > 0) {
        lines.push_back("tickrate=" + std::to_string(tick_rate));
    }
    for (const auto &l : lines) {
        games_file << l << std::endl;
    }
    return "";
}

int main(int argc, char *argv[]) {
    bool verbose = false;
    // Parse --path argument and set singleton
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--path" && i + 1 < argc) {
            BasePath::instance().set(argv[++i]);
        }
    }

    INIReader config_reader(BasePath::instance().file("config.ini"));

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--logout") {
            std::string jwt_path = get_jwt_path();
            if (std::filesystem::exists(jwt_path)) {
                std::filesystem::remove(jwt_path);
                std::cout << "Logged out." << std::endl;
            } else {
                std::cout << "No JWT file found. Already logged out." << std::endl;
            }
            return 0;
        } else if (arg == "--generate-config") {
            std::cout << "Generating config.ini" << std::endl;
            std::ofstream config_file(BasePath::instance().file("config.ini"));
            if (config_file.is_open()) {
                config_file << default_config;
                config_file.close();
            } else {
                std::cerr << "Failed to create config.ini" << std::endl;
            }
            return 0;
        } else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Usage: " << argv[0]
                << " [--verbose] [--disable-metrics] [--logout] [--generate-config] [--path <path>]"
                << std::endl;
            return 0;
        }
    }

    bool db_enabled = config_reader.GetBoolean("database", "enabled", false);
    Database db;
    if (db_enabled) {
        db.connect_to_db();
    } else {
        std::cout << "Database is disabled. Leaderboards is disabled." << std::endl;
    }
    int port = config_reader.GetUnsigned("webserverserver", "port", 8080);
    std::atomic<bool> stop(false);
    long message_queue_length =
        config_reader.GetInteger("webserverserver", "message_queue_length", long(250000));
    if (message_queue_length <= 100) {
        message_queue_length = 100;
    }
    moodycamel::BlockingReaderWriterQueue<WebSocketReceivedMessage> receive_queue(
        message_queue_length);
    moodycamel::BlockingReaderWriterQueue<DatabaseReceivedMessage> &database_queue =
        config_reader.GetBoolean("database", "enabled", false)
            ? *new moodycamel::BlockingReaderWriterQueue<DatabaseReceivedMessage>(
                  message_queue_length)
            : *new moodycamel::BlockingReaderWriterQueue<DatabaseReceivedMessage>(0);
    std::cout << "Starting webserver" << std::endl;
    uWS::App app = uWS::App();
    WebSocketServer webserver(
        verbose, config_reader.GetString("webserverserver", "log_folder", "logs"), receive_queue,
        config_reader.GetInteger("webserverserver", "max_messages_per_second", 5),
        config_reader.GetInteger("webserverserver", "max_users", 10000), app.getLoop());
    app.ws<PerSocketData>(
           "/connect",
           {/* Settings */
            .compression = static_cast<uWS::CompressOptions>(
                config_reader.GetUnsigned("webserverserver", "compression", uWS::DISABLED)),
            // max 2 kb
            .maxPayloadLength = static_cast<unsigned int>(
                config_reader.GetUnsigned("webserverserver", "max_payload_length", 2 * 1024)),
            // 30 seconds
            .idleTimeout = static_cast<unsigned short>(
                config_reader.GetUnsigned("webserverserver", "idle_timeout", 30)),
            // 64 kb
            .maxBackpressure = static_cast<unsigned int>(
                config_reader.GetUnsigned("webserverserver", "max_backpressure", 64 * 1024)),
            .closeOnBackpressureLimit = true,
            .resetIdleTimeoutOnSend =
                config_reader.GetBoolean("webserverserver", "reset_idle_timeout_on_send", true),
            .sendPingsAutomatically = true,
            /* Handlers */
            .upgrade = [&](auto *res, auto *req,
                           auto *context) { webserver.on_upgrade(res, req, context); },
            .open = [&](auto *ws) { webserver.on_open(ws); },
            .message = [&](auto *ws, std::string_view message,
                           uWS::OpCode opCode) { webserver.on_message(ws, message, opCode); },
            .close = [&](auto *ws, int code,
                         std::string_view message) { webserver.on_close(ws, message, code); }})
        .listen(port, [&](auto *listen_socket) {
            if (listen_socket) {
                std::cout << "Listening on port " << port << std::endl;
            } else {
                std::cout << "Failed to listen on port" << port << std::endl;
                stop = true;
                exit(1);
            }
        });
    GameThread GameThread(
        db_enabled, verbose,
        BasePath::instance().file(config_reader.GetString("games", "log_folder", "logs")),
        BasePath::instance().file(config_reader.Get("games", "scripts_folder", "scripts")),
        receive_queue, database_queue, app.getLoop(), &webserver, stop,
        config_reader.GetInteger("games", "listing_interval", 3000),
        config_reader.GetInteger("games", "max_reconnection_time", 6 * 60 * 1000));
    app.get("/", [](auto *res, auto *req) { res->writeStatus("200 OK")->end("OK"); });
    app.get("/health", [](auto *res, auto *req) { res->writeStatus("200 OK")->end("OK"); });
    app.post("/system/shutdown", [&stop](auto *res, auto *req) {
        res->writeStatus("200 OK")->end("OK");
        return;
        // No-op for now
        stop = true;
        // TODO do it correctly
        exit(0);
    });
    app.post("/system/notify", [&webserver](auto *res, auto *req) {
        res->writeStatus("200 OK")->end("OK");
        // No-op for now
        return;
        webserver.send_all("notify", uWS::OpCode::TEXT);
        res->writeStatus("200 OK")->end("OK");
    });
    app.post("/game/:game_id", [&webserver, &GameThread](auto *res, auto *req) {
        res->writeStatus("200 OK")->end("OK");
        // No-op for now
        return;
        std::string game_id{req->getParameter(0)};
        if (game_id.empty()) {
            res->writeStatus("400 Bad Request")->end("Game ID is required");
            return;
        }
        auto body = std::make_shared<std::string>();
        auto isAborted = std::make_shared<bool>(false);
        res->onData([res, isAborted, body, &GameThread, game_id](std::string_view chunk,
                                                                 bool isFin) mutable {
            body->append(chunk);
            if (isFin && !*isAborted) {
                std::string msg = extract_data_from_new_game(*body, game_id);
                if (msg.size() > 0) {
                    res->writeStatus("200 OK")->end(msg);
                    return;
                }
                GameThread.load_games();
                res->writeStatus("200 OK")->end("Game created");
            }
        });
        res->onAborted([isAborted]() { *isAborted = true; });
    });
    app.del("/game/:game_id", [&webserver, &GameThread](auto *res, auto *req) {
        res->writeStatus("200 OK")->end("OK");
        // No-op for now
        return;
        std::string game_id{req->getParameter(0)};
        if (game_id.empty()) {
            res->writeStatus("400 Bad Request")->end("Game ID is required");
            return;
        }
        GameThread.unload_game(game_id);
        std::vector<std::string> lines = get_game_data_without_game_id(game_id);
        // write lines back to the file
        std::ofstream games_file("games.ini", std::ios::out);
        if (!games_file.is_open()) {
            res->writeStatus("500 Internal Server Error")->end("Failed to open games.ini.");
            return;
        }
        for (const auto &l : lines) {
            games_file << l << std::endl;
        }
        res->writeStatus("200 OK")->end("Game unloaded");
    });
    app.get("/game/:game_id/leaderboard/:leaderboard_id", [&db, db_enabled](auto *res, auto *req) {
        if (!db_enabled) {
            res->writeStatus("503 Service Unavailable")->end("Database is disabled");
            return;
        }
        std::string game_id{req->getParameter(0)};
        if (game_id.empty()) {
            res->writeStatus("400 Bad Request")->end("Game ID is required");
            return;
        }
        int leaderboard_size = 10;
        int count = 0;
        auto leaderboard_size_str = req->getQuery("size");
        auto count_str = req->getQuery("count");
        if (!leaderboard_size_str.empty()) {
            leaderboard_size =
                std::min(std::max(1, std::stoi(std::string(leaderboard_size_str))), 100);
        }
        if (!count_str.empty()) {
            count = std::max(0, std::stoi(std::string(count_str)));
        }
        std::string leaderboard_id{req->getParameter(1)};
        auto top_players =
            db.leaderboard_get_top(leaderboard_id, game_id, leaderboard_size + count);
        std::vector<std::tuple<std::string, int64_t, std::string>> paged_players;
        for (int i = count; i < std::min((int)top_players.size(), count + leaderboard_size); ++i) {
            paged_players.push_back(top_players[i]);
        }
        std::string json = "[";
        for (size_t i = 0; i < paged_players.size(); ++i) {
            const auto &[user_id, score, timestamp] = paged_players[i];
            json += "{\"user_id\":\"" + user_id + "\",\"score\":" + std::to_string(score) +
                    ",\"timestamp\":\"" + timestamp + "\", \"rank\":" + std::to_string(i + 1 +) +
                    "}";
            // Add a comma if not the last element
            if (i + 1 < paged_players.size()) {
                json += ",";
            }
        }
        json += "]";
        res->writeStatus("200 OK")->end(json);
    });
    app.get("/game/:game_id/leaderboard/:leaderboard_id/user/:user_id", [&db, db_enabled](
                                                                            auto *res, auto *req) {
        if (!db_enabled) {
            res->writeStatus("503 Service Unavailable")->end("Database is disabled");
            return;
        }
        std::string game_id{req->getParameter(0)};
        if (game_id.empty()) {
            res->writeStatus("400 Bad Request")->end("Game ID is required");
            return;
        }
        std::string leaderboard_id{req->getParameter(1)};
        std::string user_id{req->getParameter(2)};
        auto player_result = db.leaderboard_get_user_score(leaderboard_id, game_id, user_id);
        const auto &[score, rank, timestamp] = player_result;
        std::string json = "{";
        json += "\"user_id\":\"" + user_id + "\",\"score\":" + std::to_string(score) +
                ",\"rank\":" + std::to_string(rank) + ",\"timestamp\":\"" + timestamp + "\"}";
        res->writeStatus("200 OK")->end(json);
    });
    std::thread GameThread_thread = std::thread([&]() { GameThread.run(); });
    std::thread GameThread_time_thread = std::thread([&]() { GameThread.time_run(); });
    app.run();
    GameThread_thread.join();
    GameThread_time_thread.join();
    if (db_enabled) {
        db.close_connection();
    }
}
