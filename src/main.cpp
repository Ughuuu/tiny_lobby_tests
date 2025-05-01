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
#include "login_client.h"
#include "pogr_client.h"
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

bool login_server() {
    std::string base_path = getUserAppDataPath("LobbyServer");
    if (!std::filesystem::exists(base_path)) {
        std::filesystem::create_directories(base_path);
    }
    std::string jwt_path = get_jwt_path();
    if (std::filesystem::exists(jwt_path)) {
        std::ifstream jwt_file(jwt_path);
        if (jwt_file.is_open()) {
            std::string jwt;
            std::getline(jwt_file, jwt);
            jwt_file.close();
            if (!jwt.empty()) {
                std::cout << "JWT already exists at " << jwt_path << std::endl;
                return true;
            }
        }
    }
    LoginClient client("login.blazium.app", "f3c31f25-b3b4-4241-908a-bab2509e0a61");
    std::string error;
    if (!client.connect(error)) {
        std::cerr << "Connect failed: " << error << std::endl;
        return false;
    }
    std::string url, login_type;
    if (client.request_url("discord", url, login_type, error)) {
        std::cout << "Login to use the Lobby Server to the following URL: " << url << std::endl;
    } else {
        std::cerr << "Request URL failed: " << error << std::endl;
        return false;
    }
    std::string jwt, type, access_token;
    if (client.wait_for_jwt(jwt, type, access_token, error)) {
        std::cout << "JWT saved to " << jwt_path << std::endl;
        std::ofstream jwt_file(jwt_path);
        if (jwt_file.is_open()) {
            jwt_file << jwt;
            jwt_file.close();
        } else {
            std::cerr << "Failed to open JWT file for writing." << std::endl;
            return false;
        }
    } else {
        std::cerr << "Login failed JWT wait failed: " << error << std::endl;
        return false;
    }
    client.close();
    return true;
}

int main(int argc, char *argv[]) {
    INIReader config_reader("config.ini");
    if (config_reader.ParseError() < 0) {
        std::cout << "Cannot open config.ini. To generate one run --generate-config" << std::endl;
    }
    bool verbose = false;
    bool disable_metrics = false;
    bool skip_login = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--disable-metrics") {
            disable_metrics = true;
        } else if (arg == "--skip-login") {
            skip_login = true;
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
            std::ofstream config_file("config.ini");
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
                << " [--verbose] [--disable-metrics] [--skip-login] [--logout] [--generate-config]"
                << std::endl;
            return 0;
        }
    }
    if (!disable_metrics) {
        std::cout << "This application collects anonymous usage statistics. To disable it pass "
                     "--disable-metrics"
                  << std::endl;
    }
    int max_users = 10000;
    if (!skip_login && login_server()) {
        std::cerr << "Login succeded." << std::endl;
        // exit(1);
    } else {
        max_users = 10;
        std::cout << "Login skipped or failed. Locked to 10 players." << std::endl;
    }

    if (config_reader.GetBoolean("database", "enabled", false) == true) {
        connect_to_db();
    }
    std::string license_id =
        config_reader.GetString("license", "id", "00000000-0000-0000-0000-000000000000");
    if (license_id.empty()) {
        std::cerr << "License ID not found in config.ini" << std::endl;
        exit(1);
    }
    int port = config_reader.GetUnsigned("webserverserver", "port", 8080);
    std::cout << "Starting analytics" << std::endl;
    moodycamel::BlockingReaderWriterQueue<AnalyticsEvent> analytics_queue(10000);
    std::atomic<bool> stop(false);
    POGRClient pogr_client{.analytics_queue = analytics_queue,
                           .client_id = "460add56-fbe1-47cf-aa6d-81d1197ad6c6",
                           .build_id =
                               "0a3c052be193527ce52542e6c2554697836325b85695c7dd0ef0ef88abc6f19edcb"
                               "6f4d2f445356b4b1b14edded38c8a61075390e91cbb60736005a4a634079b",
                           .association_id = license_id,
                           .stop = stop};
    pogr_client.enabled = !disable_metrics;
    if (pogr_client.enabled) {
        pogr_client.init();
        pogr_client.data(boost::container::flat_map<std::string, AnyElement>{
            {"os", AnyElement{POGRClient::get_os_name()}},
            {"arch", AnyElement{POGRClient::get_arch_name()}}});
        pogr_client.event("init",
                          boost::container::flat_map<std::string, AnyElement>{
                              {"os", AnyElement{POGRClient::get_os_name()}},
                              {"arch", AnyElement{POGRClient::get_arch_name()}}},
                          "server_started", "server_started", "server", "created");
    }
    long message_queue_length =
        config_reader.GetInteger("webserverserver", "message_queue_length", long(250000));
    if (message_queue_length <= 100) {
        message_queue_length = 100;
    }
    moodycamel::BlockingReaderWriterQueue<WebSocketReceivedMessage> receive_queue(
        message_queue_length);
    if (config_reader.GetBoolean("ssl", "enabled", false)) {
        std::cout << "Starting webserver with SSL" << std::endl;
        WebSocketServer<true> webserver(
            verbose, config_reader.GetString("webserverserver", "log_folder", "logs"),
            receive_queue,
            config_reader.GetInteger("webserverserver", "max_messages_per_second", 5), max_users);
        uWS::SSLApp app =
            uWS::SSLApp(uWS::SocketContextOptions{
                            .key_file_name = config_reader.Get("ssl", "key_filename", "").c_str(),
                            .cert_file_name = config_reader.Get("ssl", "cert_filename", "").c_str(),
                            .passphrase = config_reader.Get("ssl", "passphrase", "").c_str()})
                .ws<PerSocketData>(
                    "/connect",
                    {/* Settings */
                     .compression = static_cast<uWS::CompressOptions>(config_reader.GetUnsigned(
                         "webserverserver", "compression", uWS::DISABLED)),
                     // max 2 kb
                     .maxPayloadLength = static_cast<unsigned int>(config_reader.GetUnsigned(
                         "webserverserver", "max_payload_length", 2 * 1024)),
                     // 2 minutes
                     .idleTimeout = static_cast<unsigned short>(
                         config_reader.GetUnsigned("webserverserver", "idle_timeout", 120)),
                     // 64 kb
                     .maxBackpressure = static_cast<unsigned int>(config_reader.GetUnsigned(
                         "webserverserver", "max_backpressure", 64 * 1024)),
                     .closeOnBackpressureLimit = true,
                     .resetIdleTimeoutOnSend = config_reader.GetBoolean(
                         "webserverserver", "reset_idle_timeout_on_send", true),
                     .sendPingsAutomatically = true,
                     /* Handlers */
                     .upgrade = [&](auto *res, auto *req,
                                    auto *context) { webserver.on_upgrade(res, req, context); },
                     .open = [&](auto *ws) { webserver.on_open(ws); },
                     .message =
                         [&](auto *ws, std::string_view message, uWS::OpCode opCode) {
                             webserver.on_message(ws, message, opCode);
                         },
                     .drain = [](auto * /*ws*/) {},
                     .ping = [](auto * /*ws*/, std::string_view) {},
                     .pong = [](auto * /*ws*/, std::string_view) {},
                     .close =
                         [&](auto *ws, int code, std::string_view message) {
                             webserver.on_close(ws, message, code);
                         }})
                .listen(port, [&](auto *listen_socket) {
                    if (listen_socket) {
                        std::cout << "Listening on port " << port << std::endl;
                    } else {
                        std::cout << "Failed to listen on port" << port << std::endl;
                    }
                });
        app.get("/health", [](auto *res, auto *req) { res->writeStatus("200 OK")->end("OK"); });
        app.post("/system/shutdown", [&stop](auto *res, auto *req) {
            stop = true;
            res->writeStatus("200 OK")->end("OK");
            // TODO do it correctly
            exit(0);
        });
        app.post("/system/notify", [&webserver](auto *res, auto *req) {
            webserver.send_all("notify", uWS::OpCode::TEXT);
            res->writeStatus("200 OK")->end("OK");
        });
        GameThread GameThread(
            verbose, config_reader.GetString("games", "log_folder", "logs"),
            config_reader.Get("games", "scripts_folder", "scripts"), analytics_queue, receive_queue,
            app.getLoop(), &webserver, nullptr, stop,
            config_reader.GetInteger("games", "listing_interval", 3000),
            config_reader.GetInteger("games", "max_reconnection_time", 6 * 60 * 1000));
        std::thread GameThread_thread = std::thread([&]() { GameThread.run(); });
        std::thread GameThread_time_thread = std::thread([&]() { GameThread.time_run(); });
        std::thread AnalyticsThread_thread = std::thread([&]() { pogr_client.run(); });
        app.run();
        GameThread_thread.join();
        GameThread_time_thread.join();
        AnalyticsThread_thread.join();
    } else {
        std::cout << "Starting webserver without SSL" << std::endl;
        WebSocketServer<false> webserver(
            verbose, config_reader.GetString("webserverserver", "log_folder", "logs"),
            receive_queue,
            config_reader.GetInteger("webserverserver", "max_messages_per_second", 5), max_users);
        uWS::App app =
            uWS::App()
                .ws<PerSocketData>(
                    "/connect",
                    {/* Settings */
                     .compression = static_cast<uWS::CompressOptions>(config_reader.GetUnsigned(
                         "webserverserver", "compression", uWS::DISABLED)),
                     // max 2 kb
                     .maxPayloadLength = static_cast<unsigned int>(config_reader.GetUnsigned(
                         "webserverserver", "max_payload_length", 2 * 1024)),
                     // 2 minutes
                     .idleTimeout = static_cast<unsigned short>(
                         config_reader.GetUnsigned("webserverserver", "idle_timeout", 120)),
                     // 64 kb
                     .maxBackpressure = static_cast<unsigned int>(config_reader.GetUnsigned(
                         "webserverserver", "max_backpressure", 64 * 1024)),
                     .closeOnBackpressureLimit = true,
                     .resetIdleTimeoutOnSend = config_reader.GetBoolean(
                         "webserverserver", "reset_idle_timeout_on_send", true),
                     .sendPingsAutomatically = true,
                     /* Handlers */
                     .upgrade = [&](auto *res, auto *req,
                                    auto *context) { webserver.on_upgrade(res, req, context); },
                     .open = [&](auto *ws) { webserver.on_open(ws); },
                     .message =
                         [&](auto *ws, std::string_view message, uWS::OpCode opCode) {
                             webserver.on_message(ws, message, opCode);
                         },
                     .drain = [](auto * /*ws*/) {},
                     .ping = [](auto * /*ws*/, std::string_view) {},
                     .pong = [](auto * /*ws*/, std::string_view) {},
                     .close =
                         [&](auto *ws, int code, std::string_view message) {
                             webserver.on_close(ws, message, code);
                         }})
                .listen(port, [&](auto *listen_socket) {
                    if (listen_socket) {
                        std::cout << "Listening on port " << port << std::endl;
                    } else {
                        std::cout << "Failed to listen on port" << port << std::endl;
                    }
                });
        app.get("/health", [](auto *res, auto *req) { res->writeStatus("200 OK")->end("OK"); });
        app.post("/system/shutdown", [&stop](auto *res, auto *req) {
            stop = true;
            res->writeStatus("200 OK")->end("OK");
            // TODO do it correctly
            exit(0);
        });
        app.post("/system/notify", [&webserver](auto *res, auto *req) {
            webserver.send_all("notify", uWS::OpCode::TEXT);
            res->writeStatus("200 OK")->end("OK");
        });
        GameThread GameThread(
            verbose, config_reader.GetString("games", "log_folder", "logs"),
            config_reader.Get("games", "scripts_folder", "scripts"), analytics_queue, receive_queue,
            app.getLoop(), nullptr, &webserver, stop,
            config_reader.GetInteger("games", "listing_interval", 3000),
            config_reader.GetInteger("games", "max_reconnection_time", 6 * 60 * 1000));
        std::thread GameThread_thread = std::thread([&]() { GameThread.run(); });
        std::thread GameThread_time_thread = std::thread([&]() { GameThread.time_run(); });
        std::thread AnalyticsThread_thread = std::thread([&]() { pogr_client.run(); });
        app.run();
        GameThread_thread.join();
        GameThread_time_thread.join();
        AnalyticsThread_thread.join();
    }
    if (config_reader.GetBoolean("database", "enabled", false) == true) {
        close_connection();
    }
}
