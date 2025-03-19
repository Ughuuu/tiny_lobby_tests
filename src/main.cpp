#include "App.h"
#include <thread>
#include "websocket_server.h"
#include "game_thread.h"
#include <readerwriterqueue.h>
#include "INIReader.h"

int main(int argc, char* argv[]) {
    INIReader config_reader("config.ini");
    if (config_reader.ParseError() < 0) {
        std::cout << "Cannot open config.ini" << std::endl;
    }
    std::vector<std::string> args(argv + 1, argv + argc);
    bool verbose = false;

    for (const auto& arg : args) {
        if (arg == "--verbose") {
            verbose = true;
        }
    }

    int port = config_reader.GetUnsigned("webserverserver", "port", 9001);
    std::cout<< "Starting webserver on " << port << std::endl;
    
    moodycamel::BlockingReaderWriterQueue<WebSocketMessage> message_queue(1024 * 10);
    if (config_reader.GetBoolean("ssl", "enabled", false)) {
        WebSocketServer<false> webserver(verbose, config_reader.GetString("webserverserver", "log_folder", "logs"), message_queue);
        uWS::App app = uWS::App()
            .ws<PerSocketData>("/connect", {
                /* Settings */
                .compression = static_cast<uWS::CompressOptions>(config_reader.GetUnsigned("webserverserver", "compression", uWS::DISABLED)),
                // max 2 kb
                .maxPayloadLength = static_cast<unsigned int>(config_reader.GetUnsigned("webserverserver", "max_payload_length", 2 * 1024)),
                .resetIdleTimeoutOnSend = config_reader.GetBoolean("webserverserver", "reset_idle_timeout_on_send", true),
                .closeOnBackpressureLimit = true,
                // 3 minutes
                .idleTimeout = static_cast<unsigned short>(config_reader.GetUnsigned("webserverserver", "idle_timeout", 180)),
                // 64 kb
                .maxBackpressure = static_cast<unsigned int>(config_reader.GetUnsigned("webserverserver", "max_backpressure", 64 * 1024)),
                /* Handlers */
                .upgrade = [&](auto *res, auto *req, auto *context) {
                    webserver.on_upgrade(res, req, context);
                },
                .open = [&](auto *ws) {
                    webserver.on_open(ws);
                },
                .message = [&](auto *ws, std::string_view message, uWS::OpCode opCode) {
                    webserver.on_message(ws, message, opCode);
                },
                .drain = [](auto */*ws*/) {
                },
                .ping = [](auto */*ws*/, std::string_view) {
                },
                .pong = [](auto */*ws*/, std::string_view) {
                },
                .close = [&](auto *ws, int code, std::string_view message) {
                    webserver.on_close(ws, message, code);
                }
            })
            .listen(port, [&](auto *listen_socket) {
                if (listen_socket) {
                    std::cout << "Listening on port " << port << std::endl;
                } else {
                    std::cout << "Failed to listen on port" << port << std::endl;
                }
            });
        std::thread GameThread_thread = std::thread([&]() {
            GameThread GameThread(verbose, config_reader.GetString("game", "log_folder", "logs"), config_reader.Get("game", "scripts_folder", ""), message_queue, app.getLoop(), nullptr, &webserver);
            GameThread.run();
        });
        app.run();
        GameThread_thread.join();
    } else {
        WebSocketServer<true> webserver(verbose, config_reader.GetString("webserverserver", "log_folder", "logs"), message_queue);
        uWS::SSLApp app = uWS::SSLApp(uWS::SocketContextOptions {
                .key_file_name = config_reader.Get("ssl", "key_filename", "").c_str(),
                .cert_file_name = config_reader.Get("ssl", "cert_filename", "").c_str(),
                .passphrase = config_reader.Get("ssl", "passphrase", "").c_str()
            }).ws<PerSocketData>("/connect", {
                /* Settings */
                .compression = static_cast<uWS::CompressOptions>(config_reader.GetUnsigned("webserverserver", "compression", uWS::DISABLED)),
                // max 2 kb
                .maxPayloadLength = static_cast<unsigned int>(config_reader.GetUnsigned("webserverserver", "max_payload_length", 2 * 1024)),
                .resetIdleTimeoutOnSend = config_reader.GetBoolean("webserverserver", "reset_idle_timeout_on_send", true),
                .closeOnBackpressureLimit = true,
                // 3 minutes
                .idleTimeout = static_cast<unsigned short>(config_reader.GetUnsigned("webserverserver", "idle_timeout", 180)),
                // 64 kb
                .maxBackpressure = static_cast<unsigned int>(config_reader.GetUnsigned("webserverserver", "max_backpressure", 64 * 1024)),
                /* Handlers */
                .upgrade = [&](auto *res, auto *req, auto *context) {
                    webserver.on_upgrade(res, req, context);
                },
                .open = [&](auto *ws) {
                    webserver.on_open(ws);
                },
                .message = [&](auto *ws, std::string_view message, uWS::OpCode opCode) {
                    webserver.on_message(ws, message, opCode);
                },
                .drain = [](auto */*ws*/) {
                },
                .ping = [](auto */*ws*/, std::string_view) {
                },
                .pong = [](auto */*ws*/, std::string_view) {
                },
                .close = [&](auto *ws, int code, std::string_view message) {
                    webserver.on_close(ws, message, code);
                }
            })
            .listen(port, [&](auto *listen_socket) {
                if (listen_socket) {
                    std::cout << "Listening on port " << port << std::endl;
                } else {
                    std::cout << "Failed to listen on port" << port << std::endl;
                }
            });
        std::thread GameThread_thread = std::thread([&]() {
            GameThread GameThread(verbose, config_reader.GetString("game", "log_folder", "logs"), config_reader.Get("game", "scripts_folder", ""), message_queue, app.getLoop(), &webserver, nullptr);
            GameThread.run();
        });
        app.run();
        GameThread_thread.join();
    }
}
