#include "App.h"
#include <thread>
#include "websocket_server.h"
#include "game_server.h"
#include <boost/lockfree/queue.hpp>
#include <readerwriterqueue.h>

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    bool verbose = false;

    for (const auto& arg : args) {
        if (arg == "--verbose") {
            verbose = true;
        }
    }

    int port = 9001;
    std::cout<< "Starting webserver on " << port << std::endl;
    moodycamel::BlockingReaderWriterQueue<WebSocketMessage> message_queue(100);
    WebSocketServer webserver(verbose, message_queue);
    uWS::App app = uWS::App()
        .ws<PerSocketData>("/connect", {
            /* Settings */
            .compression = uWS::SHARED_COMPRESSOR,
            .maxPayloadLength = 16 * 1024,
            .resetIdleTimeoutOnSend = true,
            // 3 minutes
            .idleTimeout = 180,
            .maxBackpressure = 1 * 1024 * 1024,
            /* Handlers */
            .upgrade = nullptr,
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
    uWS::App *shared_app = &app;
    std::thread gameserver_thread = std::thread([&]() {
        GameServer gameserver(verbose, message_queue, shared_app->getLoop(), &webserver);
        gameserver.run();
    });
    app.run();
    gameserver_thread.join();
}
