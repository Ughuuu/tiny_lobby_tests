#include "websocket_server.h"
#include "any_type.h"

#include <string>
#include <algorithm>
#include <cctype>
#include <atomic>

// 1 second
const int64_t RATE_LIMIT_WINDOW = 1000;

static inline int64_t get_time_now() {
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
    if (connected_users > max_users) {
        logger.error_log("[WebSocketServer] error: too many users");
        res->writeStatus("400 Bad Request")->write("Too many users.");
        res->end();
        return;
    }
    std::string protocol(req->getHeader("sec-websocket-protocol"));
    std::stringstream ss(protocol);
    boost::container::vector<std::string> protocols_split;

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
        .uid = to_string(gen()),
        .id = small_uuid,
        .game_id = protocols_split[1]
    };
    if (protocols_split.size() > 2) {
        user_data.reconnection_token = protocols_split[2];
    }
    logger.debug_log("[WebSocketServer] on_upgrade: ", user_data.uid, " ", user_data.id, " ", user_data.game_id, protocol);
    res->template upgrade<PerSocketData>(std::move(user_data),
        req->getHeader("sec-websocket-key"),
        "blazium",
        req->getHeader("sec-websocket-extensions"),
        context);
}

template <bool SSL>
void WebSocketServer<SSL>::on_open(uWS::WebSocket<SSL, true, PerSocketData> *ws) {
    PerSocketData* data = ws->getUserData();
    logger.debug_log("[WebSocketServer] on_open: ", data->uid, " ", data->id, " ", data->game_id);
    if (data->reconnection_token != "" &&
        reconnections.find(data->reconnection_token) != reconnections.end()) {
        auto &old_reconnection = reconnections[data->reconnection_token];
        if (old_reconnection.timestamp < get_time_now() - MAX_RECONNECTION_TIME) {
            reconnections.erase(data->reconnection_token);
            send(data->id, std::string("Reconnect expired"), uWS::OpCode::CLOSE);
            return;
        }
        auto &old_id = old_reconnection.peer_id;
        if (connection_data.find(old_id) == connection_data.end()) {
            reconnections.erase(data->reconnection_token);
            send(data->id, std::string("Reconnect Peer ID Mismatch"), uWS::OpCode::CLOSE);
            return;
        }
        auto &old_connection_data = connection_data[old_id];
        // game id doesn't match, close new connection
        if (old_connection_data.game_id != data->game_id) {
            send(data->id, std::string("Reconnect Game ID Mismatch"), uWS::OpCode::CLOSE);
            return;
        }
        // old reconnection id exists, delete id
        send(old_id, std::string("Reconnect Close"), uWS::OpCode::CLOSE);
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
    connection_data.insert_or_assign(data->id, PeerConnectionData<SSL> {
        .id = data->id,
        .game_id = data->game_id,
        .reconnection_token = data->reconnection_token,
        .ws = ws,
    });
    connected_users++;
    if (!receive_queue.enqueue(WebSocketReceivedMessage {
        .id = data->id,
        .event = WebSocketEvent::OPEN,
        .message = std::string(),
        .game_id = data->game_id,
        .reconnection_token = data->reconnection_token
    })) {
        logger.error_log("[WebSocketServer] on_open error out of memory: ", data->uid, " ", data->id, " ", data->game_id);
        send(data->id, "Out of memory", uWS::OpCode::CLOSE);
    }
}
template <bool SSL>
void WebSocketServer<SSL>::on_message(uWS::WebSocket<SSL, true, PerSocketData> *ws, const std::string_view &message, uWS::OpCode opCode) {
    PerSocketData* data = ws->getUserData();
    int64_t now = get_time_now();
    if (now - data->last_message_time < RATE_LIMIT_WINDOW) {
        data->message_count++;
    } else {
        data->last_message_time = now;
        data->message_count = 1;
    }
    if (data->message_count > max_messages_per_second) {
        logger.error_log("[WebSocketServer] on_message Rate limit exceeded: ", data->uid, " ", data->id, " ", data->game_id, " ", message, " ", opCode);
        send(data->id, "Rate limit exceeded", uWS::OpCode::CLOSE);
        return;
    }
    logger.debug_log("[WebSocketServer] on_message: ", data->uid, " ", data->id, " ", data->game_id, " ", message, " ", opCode);
    if (!receive_queue.try_enqueue(WebSocketReceivedMessage {
        .id = data->id,
        .event = WebSocketEvent::MESSAGE,
        .message = std::string(message),
        .game_id = data->game_id,
        .reconnection_token = data->reconnection_token
    })) {
        logger.error_log("[WebSocketServer] on_message error out of memory: ", data->uid, " ", data->id, " ", data->game_id, " ", message, " ", opCode);
        send(data->id, "Too many queued messages", uWS::OpCode::CLOSE);
    }
}
template <bool SSL>
void WebSocketServer<SSL>::on_close(uWS::WebSocket<SSL, true, PerSocketData> *ws, const std::string_view &message, int opCode) {
    PerSocketData* data = ws->getUserData();
    // delete websocket only if reconnection_token matches
    if (connection_data.find(data->id) != connection_data.end()) {
        if (data->reconnection_token == connection_data[data->id].reconnection_token) {
            connection_data[data->id].ws = nullptr;
        }
    }
    connected_users--;
    logger.debug_log("[WebSocketServer] on_close: ", data->uid, " ", data->id, " ", data->game_id, " ", message , " ", opCode);
    receive_queue.enqueue(WebSocketReceivedMessage {
        .id = data->id,
        .event = WebSocketEvent::CLOSE,
        .message = std::string(message),
        .game_id = data->game_id,
        .reconnection_token = data->reconnection_token
    });
}

template <bool SSL>
void WebSocketServer<SSL>::send(std::string id, const std::string &message, uWS::OpCode opCode) {
    auto it = connection_data.find(id);
    if (it != connection_data.end()) {
        if (it->second.ws != nullptr) {
            PerSocketData* data = it->second.ws->getUserData();
            logger.debug_log("[WebSocketServer] on_send: ", data->uid, " ", data->id, " ", data->game_id, " ", message, " ", opCode);
            if (opCode == uWS::OpCode::CLOSE) {
                it->second.ws->end(1002, message);
                it->second.ws = nullptr;
            } else {
                it->second.ws->send(message, opCode);
            }
        } else {
            logger.debug_log("[WebSocketServer] on_send failed, ws empty: ", id, " ", message, " ", opCode);
        }
    }
}

template <bool SSL>
void WebSocketServer<SSL>::clear_users(boost::container::flat_set<std::string> users_to_clean) {
    for (auto &user_id : users_to_clean) {
        send(user_id, std::string("Failed to reconnect"), uWS::OpCode::CLOSE);
        // user didn't reconnect, delete user
        if (connection_data.find(user_id) == connection_data.end()) {
            continue;
        }
        auto &connection = connection_data[user_id];
        reconnections.erase(connection.reconnection_token);
        connection_data.erase(user_id);
    }
}

template <bool SSL>
WebSocketServer<SSL>::WebSocketServer(bool verbose,
    std::string log_folder,
    moodycamel::BlockingReaderWriterQueue<WebSocketReceivedMessage>& receive_queue,
    int max_messages_per_second) :
    logger(verbose, log_folder + "/websocket.txt"), receive_queue(receive_queue), max_messages_per_second(max_messages_per_second) {
    logger.debug_log("[WebSocketServer] on_start");
}
