#include "websocket_server.h"

void WebSocketServer::on_open(uWS::WebSocket<USES_SSL, true, PerSocketData> *ws) {
    PerSocketData* data = ws->getUserData();
    data->id = gen();
    sockets[data->id] = ws;
    logger.debug_log("[WebSocketServer] on_open: ", data->id);
    message_queue.enqueue(WebSocketMessage {
        .id = data->id,
        .message = std::string(),
        .event = WebSocketEvent::OPEN,
    });
}
void WebSocketServer::on_message(uWS::WebSocket<USES_SSL, true, PerSocketData> *ws, const std::string_view &message, uWS::OpCode opCode) {
    PerSocketData* data = ws->getUserData();
    logger.debug_log("[WebSocketServer] on_message: ", data->id, " ", message, " ", opCode);
    message_queue.enqueue(WebSocketMessage {
        .id = data->id,
        .message = std::string(message),
        .event = WebSocketEvent::MESSAGE,
    });
}
void WebSocketServer::on_close(uWS::WebSocket<USES_SSL, true, PerSocketData> *ws, const std::string_view &message, int opCode) {
    PerSocketData* data = ws->getUserData();
    sockets.erase(data->id);
    logger.debug_log("[WebSocketServer] on_close: ", data->id, " ", message, " ", opCode);
    message_queue.enqueue(WebSocketMessage {
        .id = data->id,
        .message = std::string(message),
        .event = WebSocketEvent::CLOSE,
    });
}

void WebSocketServer::send(boost::uuids::uuid id, const std::string &message, uWS::OpCode opCode) {
    auto it = sockets.find(id);
    if (it != sockets.end()) {
        it->second->send(message, opCode);
    }
}

WebSocketServer::WebSocketServer(bool verbose, moodycamel::BlockingReaderWriterQueue<WebSocketMessage>& message_queue) : logger(verbose), message_queue(message_queue) {
    logger.debug_log("[WebSocketServer] on_start");
}
