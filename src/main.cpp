#include <readerwriterqueue.h>
#include <stddef.h>
#include <uwebsockets/App.h>

#include <atomic>
#include <filesystem>
#include <thread>

#include "INIReader.h"
#include "common/any_type.h"
#include "common/base_path.h"
#include "common/default_config.h"
#include "database/database.h"
#include "game/game_thread.h"
#include "websocket/websocket_server.h"
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
            std::cout << "Usage: " << argv[0]
                      << " [--verbose] [--logout] [--generate-config] [--path <path>]" << std::endl;
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
        int leaderboard_start = 0;
        auto leaderboard_size_str = req->getQuery("size");
        auto start_str = req->getQuery("start");
        if (!leaderboard_size_str.empty()) {
            leaderboard_size =
                std::min(std::max(1, std::stoi(std::string(leaderboard_size_str))), 100);
        }
        if (!start_str.empty()) {
            leaderboard_start = std::max(0, std::stoi(std::string(start_str)));
        }
        std::string leaderboard_id{req->getParameter(1)};
        auto top_players =
            db.leaderboard_get_top(leaderboard_id, game_id, leaderboard_size, leaderboard_start);
        std::string json = "[";
        for (size_t i = 0; i < top_players.size(); ++i) {
            const auto &[user_id, score, timestamp] = top_players[i];
            json += "{\"user_id\":\"" + user_id + "\",\"score\":" + std::to_string(score) +
                    ",\"timestamp\":\"" + timestamp +
                    "\", \"rank\":" + std::to_string(i + 1 + leaderboard_start) + "}";
            // Add a comma if not the last element
            if (i + 1 < top_players.size()) {
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
