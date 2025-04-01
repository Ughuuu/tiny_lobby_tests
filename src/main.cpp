#include <readerwriterqueue.h>

#include <thread>
#include <stddef.h>

#include "App.h"
#include "INIReader.h"
#include "game_thread.h"
#include "websocket_server.h"
#include "decrypt_pgp.h"
#include "database.h"
#include "pogr_client.h"

void check_signature() {
    std::string license_data = "license_id|holder|email|created_on|expires_on";
    std::string signature_base64 = "<base64-signature>";
    //verify_detached_signature(license_data, signature_base64);
}

int main(int argc, char *argv[]) {
    init_rnp();
    import_public_key();
    check_signature();
    INIReader config_reader("config.ini");
    if (config_reader.ParseError() < 0) {
        std::cout << "Cannot open config.ini" << std::endl;
    }
    bool verbose = false;
    bool disable_metrics = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--disable-metrics") {
            disable_metrics = true;
        }
    }
    if (!disable_metrics) {
        std::cout << "This application collects anonymous usage statistics. To disable it pass --disable-metrics" << std::endl;
    }

    if (config_reader.GetBoolean("database", "enabled", false) == true) {
        connect_to_db();
    }

    int port = config_reader.GetUnsigned("webserverserver", "port", 8080);
    std::cout << "Starting analytics"<< std::endl;
    POGRClient pogr_client{
        .client_id = "460add56-fbe1-47cf-aa6d-81d1197ad6c6",
        .build_id = "0a3c052be193527ce52542e6c2554697836325b85695c7dd0ef0ef88abc6f19edcb6f4d2f445356b4b1b14edded38c8a61075390e91cbb60736005a4a634079b"
    };
    POGRClient pogr_other_client{
        .client_id = config_reader.GetString("analytics", "client_id", ""),
        .build_id = config_reader.GetString("analytics", "build_id", "")
    };
    if (!disable_metrics && pogr_client.enabled) {
        pogr_client.init();
    }
    if (pogr_other_client.enabled) {
        pogr_other_client.init();
    }
    std::flush(std::cout);
    long message_queue_length = config_reader.GetInteger("webserverserver", "message_queue_length", long(250000));
    if (message_queue_length <= 100) {
        message_queue_length = 100;
    }
    std::flush(std::cout);
    moodycamel::BlockingReaderWriterQueue<WebSocketReceivedMessage> receive_queue(message_queue_length);
    std::flush(std::cout);
    if (config_reader.GetBoolean("ssl", "enabled", false)) {
        std::cout << "Starting webserver with SSL" << std::endl;
        WebSocketServer<false> webserver(
            verbose, config_reader.GetString("webserverserver", "log_folder", "logs"),
            receive_queue, config_reader.GetInteger("webserverserver", "max_messages_per_second", 5));
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
                     // 3 minutes
                     .idleTimeout = static_cast<unsigned short>(
                         config_reader.GetUnsigned("webserverserver", "idle_timeout", 180)),
                     // 64 kb
                     .maxBackpressure = static_cast<unsigned int>(config_reader.GetUnsigned(
                         "webserverserver", "max_backpressure", 64 * 1024)),
                     .closeOnBackpressureLimit = true,
                     .resetIdleTimeoutOnSend = config_reader.GetBoolean(
                         "webserverserver", "reset_idle_timeout_on_send", true),
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
        app.get("/health", [](auto *res, auto *req) {
            res->writeStatus("200 OK")->end("OK");
        });
        std::thread GameThread_thread = std::thread([&]() {
            GameThread GameThread(verbose, config_reader.GetString("game", "log_folder", "logs"),
                                  config_reader.Get("game", "scripts_folder", "scripts"),
                                  receive_queue, app.getLoop(), nullptr, &webserver,
                                  config_reader.GetInteger("game", "listing_interval", 3000),
                                  config_reader.GetInteger("game", "max_reconnection_time", 6 * 60 * 1000));
            GameThread.run();
        });
        app.run();
        GameThread_thread.join();
    } else {
        std::cout << "Starting webserver without SSL" << std::endl;
        WebSocketServer<true> webserver(
            verbose, config_reader.GetString("webserverserver", "log_folder", "logs"),
            receive_queue, config_reader.GetInteger("webserverserver", "max_messages_per_second", 5));
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
                     // 3 minutes
                     .idleTimeout = static_cast<unsigned short>(
                         config_reader.GetUnsigned("webserverserver", "idle_timeout", 180)),
                     // 64 kb
                     .maxBackpressure = static_cast<unsigned int>(config_reader.GetUnsigned(
                         "webserverserver", "max_backpressure", 64 * 1024)),
                     .closeOnBackpressureLimit = true,
                     .resetIdleTimeoutOnSend = config_reader.GetBoolean(
                         "webserverserver", "reset_idle_timeout_on_send", true),
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
        app.get("/health", [](auto *res, auto *req) {
            res->writeStatus("200 OK")->end("OK");
        });
        std::thread GameThread_thread = std::thread([&]() {
            GameThread GameThread(verbose, config_reader.GetString("game", "log_folder", "logs"),
                                  config_reader.Get("game", "scripts_folder", "scripts"),
                                  receive_queue, app.getLoop(), &webserver, nullptr,
                                  config_reader.GetInteger("game", "listing_interval", 3000),
                                  config_reader.GetInteger("game", "max_reconnection_time", 6 * 60 * 1000));
            GameThread.run();
        });
        app.run();
        GameThread_thread.join();
    }
    deinit_rnp();
    if (config_reader.GetBoolean("database", "enabled", false) == true) {
        close_connection();
    }
}
