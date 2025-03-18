#include "websocket_server.h"

#include <string>
#include <algorithm>
#include <cctype>

int64_t get_time_now() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

static inline std::string trim(const std::string& s) {
    auto start = std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    });
    auto end = std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base();
    if (start >= end) return "";
    return std::string(start, end);
}

template <bool SSL>
void WebSocketServer<SSL>::on_upgrade(uWS::HttpResponse<SSL> *res,uWS::HttpRequest *req, struct us_socket_context_t *context) {
    std::string protocol(req->getHeader("sec-websocket-protocol"));
    logger.debug_log("[WebSocketServer] on_upgrade: ", protocol);
    std::stringstream ss(protocol);
    std::vector<std::string> protocols_split;

    while( ss.good() )
    {
        std::string substr;
        getline( ss, substr, ',' );
        protocols_split.push_back( trim(substr) );
    }

    if (protocols_split.size() < 2 || protocols_split[0] != "blazium") {
        logger.error_log("[WebSocketServer] error: ", protocol);
        res->writeStatus("400 Bad Request")->write("Failed to open WebSocket connection.");
        res->end();
        return;
    }
    auto uuid = to_string(gen());
    auto small_uuid = uuid.substr(0, 8);
    PerSocketData user_data {
        .id = small_uuid,
        .game_id = protocols_split[1]
    };
    if (protocols_split.size() > 2) {
        user_data.reconnection_token = protocols_split[2];
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
    if (data->reconnection_token != "" &&
        reconnections.find(data->reconnection_token) != reconnections.end()) {
        auto &old_reconnection = reconnections[data->reconnection_token];
        if (old_reconnection.timestamp < get_time_now() - MAX_RECONNECTION_TIME) {
            reconnections.erase(data->reconnection_token);
            send(data->id, std::string("Reconnected expired"), uWS::OpCode::CLOSE);
            return;
        }
        auto &old_id = old_reconnection.peer_id;
        auto &old_connection_data = connection_data[old_id];
        // game id doesn't match, close new connection
        if (old_connection_data.game_id != data->game_id) {
            send(data->id, std::string("Reconnected close"), uWS::OpCode::CLOSE);
            return;
        }
        // old reconnection id exists, delete id
        send(old_id, std::string("Reconnected close"), uWS::OpCode::CLOSE);
        data->id = old_id;
        reconnections.erase(data->reconnection_token);
    }
    auto uuid = to_string(gen());
    data->reconnection_token = uuid;
    // write to sockets map
    reconnections.emplace(data->reconnection_token, ReconnectionTokens {
        .peer_id = data->id,
        .timestamp = get_time_now()
    });
    connection_data.insert_or_assign(data->id, PeerConnectionData {
        .id = data->id,
        .game_id = data->game_id,
        .reconnection_token = data->reconnection_token,
        .ws = ws,
    });
    logger.debug_log("[WebSocketServer] on_open: ", data->id);
    message_queue.enqueue(WebSocketMessage {
        .id = data->id,
        .message = std::string(),
        .event = WebSocketEvent::OPEN,
        .game_id = data->game_id,
        .reconnection_token = data->reconnection_token,
    });
}
template <bool SSL>
void WebSocketServer<SSL>::on_message(uWS::WebSocket<SSL, true, PerSocketData> *ws, const std::string_view &message, uWS::OpCode opCode) {
    PerSocketData* data = ws->getUserData();
    logger.debug_log("[WebSocketServer] on_message: ", data->id, " ", message, " ", opCode);
    message_queue.enqueue(WebSocketMessage {
        .id = data->id,
        .message = std::move(std::string(message)),
        .event = WebSocketEvent::MESSAGE,
        .game_id = data->game_id,
        .reconnection_token = data->reconnection_token,
    });
}
template <bool SSL>
void WebSocketServer<SSL>::on_close(uWS::WebSocket<SSL, true, PerSocketData> *ws, const std::string_view &message, int opCode) {
    PerSocketData* data = ws->getUserData();
    // delete websocket only if reconnection_token matches
    if (data->reconnection_token == connection_data[data->id].reconnection_token) {
        connection_data[data->id].ws = nullptr;
    }
    logger.debug_log("[WebSocketServer] on_close: ", data->id, " ", message, " ", opCode);
    message_queue.enqueue(WebSocketMessage {
        .id = data->id,
        .message = std::move(std::string(message)),
        .event = WebSocketEvent::CLOSE,
        .game_id = data->game_id,
        .reconnection_token = data->reconnection_token,
    });
}

template <bool SSL>
void WebSocketServer<SSL>::send(std::string id, const std::string &message, uWS::OpCode opCode) {
    auto it = connection_data.find(id);
    if (it != connection_data.end()) {
        logger.debug_log("[WebSocketServer] send: ", id, " ", message, " ", opCode);
        if (it->second.ws != nullptr) {
            it->second.ws->send(message, opCode);
        }
    }
}

template <bool SSL>
WebSocketServer<SSL>::WebSocketServer(bool verbose,
    std::string log_folder,
    moodycamel::BlockingReaderWriterQueue<WebSocketMessage>& message_queue) : logger(verbose, log_folder + "/websocket.txt"), message_queue(message_queue) {
    logger.debug_log("[WebSocketServer] on_start");
}
