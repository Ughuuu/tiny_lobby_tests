#pragma once
#include "server_logger.h"
#include <readerwriterqueue.h>
#include "websocket_server.h"
#include "App.h"

class GameServer {
    ServerLogger logger;
    WebSocketServer *webserver;
    moodycamel::BlockingReaderWriterQueue<WebSocketMessage> &message_queue;
    struct uWS::Loop *loop;
public:
    void run();
    GameServer(bool verbose,
        moodycamel::BlockingReaderWriterQueue<WebSocketMessage> &message_queue,
        uWS::Loop *loop,
        WebSocketServer *webserver);
};
