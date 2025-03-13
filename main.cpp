#include "App.h"
#include <thread>
#include <algorithm>

struct PerSocketData {

};

class WebServer {
    void on_open(PerSocketData *ws) {

    }
};

int main() {
    WebServer webserver;
    std::thread websocket_thread = std::thread([]() {
        return new std::thread([]() {
            uWS::App().ws<PerSocketData>("/connect", {
                /* Settings */
                .compression = uWS::SHARED_COMPRESSOR,
                .maxPayloadLength = 16 * 1024,
                .idleTimeout = 6,
                .maxBackpressure = 1 * 1024 * 1024,
                /* Handlers */
                .upgrade = nullptr,
                .open = [](auto *ws) {
                    PerSocketData* per_socket_data = ws->getUserData();
                    //per_socket_data->id =
                    //webserver.on_open(ws->getUserData());
                    std::cout<<" open" << std::endl;
                },
                .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {
                    ws->send(message, opCode);
                },
                .drain = [](auto */*ws*/) {
                    /* Check getBufferedAmount here */
                },
                .ping = [](auto */*ws*/, std::string_view) {

                },
                .pong = [](auto */*ws*/, std::string_view) {

                },
                .close = [](auto */*ws*/, int /*code*/, std::string_view /*message*/) {

                }
            }).listen(9001, [](auto *listen_socket) {
                if (listen_socket) {
                    std::cout << "Thread " << std::this_thread::get_id() << " listening on port " << 9001 << std::endl;
                } else {
                    std::cout << "Thread " << std::this_thread::get_id() << " failed to listen on port 9001" << std::endl;
                }
            }).run();

        });
    });
    websocket_thread.join();
}
