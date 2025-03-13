#include "game_server.h"
#include <simdjson.h>

GameServer::GameServer(bool verbose,
    moodycamel::BlockingReaderWriterQueue<WebSocketMessage> &message_queue,
    uWS::Loop *loop,
    WebSocketServer *webserver): logger(verbose), message_queue(message_queue), loop(loop), webserver(webserver) {
    logger.debug_log("[GameServer] on_start");
}

void GameServer::run() {
    // max 10kb
    simdjson::ondemand::parser parser(10000);
    logger.debug_log("[GameServer] on_run");
    while (true) {
        WebSocketMessage message;
        message_queue.wait_dequeue(message);
        simdjson::ondemand::document doc;
        simdjson::padded_string padded_string(message.message);
        auto error = parser.iterate(padded_string).get(doc);
        if(error) {
            logger.debug_log("[GameServer] error: ", message.id, " ", message.event, " ", simdjson::error_message(error));
            continue;
        }
        logger.debug_log("[GameServer] on_run: ", message.id, " ", message.event);
        loop->defer([id = message.id, msg = message.message, webserver = webserver]() {
            webserver->send(id, msg, uWS::OpCode::TEXT);
        });
    }
}
