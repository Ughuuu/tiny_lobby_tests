#include "websocket_server.h"

template <bool SSL>
void WebSocketServer<SSL>::on_upgrade(uWS::HttpResponse<SSL> *res, uWS::HttpRequest *req, struct us_socket_context_t *context) {
    std::string protocol(req->getHeader("sec-websocket-protocol"));
    logger.debug_log("[WebSocketServer] on_upgrade: ", protocol);
    std::stringstream ss(protocol);
    std::vector<std::string> protocols_split;

    while( ss.good() )
    {
        std::string substr;
        getline( ss, substr, ',' );
        protocols_split.push_back( substr );
    }

    if (protocols_split.size() < 2 || protocols_split[0] != "blazium") {
        logger.error_log("[WebSocketServer] error: ", protocol);
        res->writeStatus("400 Bad Request")->write("Failed to open WebSocket connection.");
        res->end();
        return;
    }
    PerSocketData user_data {
        .id = to_string(gen()),
        .game_id = protocols_split[1]
    };
    if (protocols_split.size() > 2) {
        user_data.reconnection_id = protocols_split[2];
    }
    res->template upgrade<PerSocketData>(std::move(user_data),
        req->getHeader("sec-websocket-key"),
        "blazium",
        req->getHeader("sec-websocket-extensions"),
        context);
}

template <bool SSL>
void WebSocketServer<SSL>::on_open(uWS::WebSocket<SSL, true, PerSocketData> *ws) {
    PerSocketData* data = ws->getUserData();
    // write to sockets map
    sockets[data->id] = ws;
    logger.debug_log("[WebSocketServer] on_open: ", data->id);
    message_queue.enqueue(WebSocketMessage {
        .id = data->id,
        .message = std::string(),
        .event = WebSocketEvent::OPEN,
        .game_id = data->game_id,
        .reconnection_id = data->reconnection_id
    });
}
template <bool SSL>
void WebSocketServer<SSL>::on_message(uWS::WebSocket<SSL, true, PerSocketData> *ws, const std::string_view &message, uWS::OpCode opCode) {
    PerSocketData* data = ws->getUserData();
    logger.debug_log("[WebSocketServer] on_message: ", data->id, " ", message, " ", opCode);
    message_queue.enqueue(WebSocketMessage {
        .id = data->id,
        .message = std::string(message),
        .event = WebSocketEvent::MESSAGE,
    });
}
template <bool SSL>
void WebSocketServer<SSL>::on_close(uWS::WebSocket<SSL, true, PerSocketData> *ws, const std::string_view &message, int opCode) {
    PerSocketData* data = ws->getUserData();
    sockets.erase(data->id);
    logger.debug_log("[WebSocketServer] on_close: ", data->id, " ", message, " ", opCode);
    message_queue.enqueue(WebSocketMessage {
        .id = data->id,
        .message = std::string(message),
        .event = WebSocketEvent::CLOSE,
    });
}

template <bool SSL>
void WebSocketServer<SSL>::send(std::string id, const std::string &message, uWS::OpCode opCode) {
    auto it = sockets.find(id);
    if (it != sockets.end()) {
        it->second->send(message, opCode);
    }
}

template <bool SSL>
WebSocketServer<SSL>::WebSocketServer(bool verbose, moodycamel::BlockingReaderWriterQueue<WebSocketMessage>& message_queue) : logger(verbose), message_queue(message_queue) {
    logger.debug_log("[WebSocketServer] on_start");
}
