#pragma once
#include <readerwriterqueue.h>
#include <uwebsockets/App.h>

#include <boost/container/flat_set.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "../authentication/authentication_thread.h"
#include "../database/database.h"
#include "../common/server_logger.h"
#include "per_socket_data.h"

std::string ERROR_SERVER_TOO_MANY_USERS = "ERROR_SERVER_TOO_MANY_USERS";
std::string ERROR_SERVER_FAILED_TO_OPEN_WEBSOCKET = "ERROR_SERVER_FAILED_TO_OPEN_WEBSOCKET";
std::string ERROR_SERVER_QUEUE_FULL = "ERROR_SERVER_QUEUE_FULL";
std::string ERROR_SERVER_RATE_LIMIT = "ERROR_SERVER_RATE_LIMIT";
std::string ERROR_SERVER_RECONNECT_EXPIRED = "ERROR_SERVER_RECONNECT_EXPIRED";
std::string ERROR_SERVER_RECONNECT_PEER_ID_NOT_FOUND = "ERROR_SERVER_RECONNECT_PEER_ID_NOT_FOUND";
std::string ERROR_SERVER_WRONG_GAMEID = "ERROR_SERVER_WRONG_GAMEID";
std::string ERROR_SERVER_RECONNECT_CLOSE = "ERROR_SERVER_RECONNECT_CLOSE";
std::string ERROR_SERVER_FAILED_TO_RECONNECT = "ERROR_SERVER_FAILED_TO_RECONNECT";
std::string ERROR_SERVER_SHUTTING_DOWN = "ERROR_SERVER_SHUTTING_DOWN";

static inline int64_t get_time_now() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

struct PeerConnectionData {
    std::string id;
    std::string game_id;
    std::string reconnection_token;
    uWS::WebSocket<false, true, PerSocketData> *ws;
};

enum WebSocketEvent { OPEN, MESSAGE, CLOSE };

struct WebSocketReceivedMessage {
    std::string id;
    WebSocketEvent event;
    std::string message;
    std::string game_id;
    std::string reconnection_token;
    std::string platform = "anon";

    std::string name;
    std::string platform_id;
};


struct ReconnectionTokens {
    std::string peer_id;
    int64_t timestamp;
};

class WebSocketServer {
    int MAX_RECONNECTION_TIME = 6 * 60 * 1000;
    int max_users;
    int max_messages_per_second;
    int connected_users = 0;
    boost::uuids::random_generator gen;
    moodycamel::BlockingReaderWriterQueue<WebSocketReceivedMessage> &receive_queue;
    moodycamel::BlockingReaderWriterQueue<AuthenticationMessage> authentication_queue;
    ServerLogger logger;
    std::unordered_map<std::string, PeerConnectionData> connection_data;
    std::unordered_map<std::string, ReconnectionTokens> reconnections;
    AuthenticationThread authentication_thread;
    std::thread authentication_thread_handle;
    struct uWS::Loop *loop;

   public:
    void on_upgrade(uWS::HttpResponse<false> *res, uWS::HttpRequest *req,
                    struct us_socket_context_t *context);
    void on_open(uWS::WebSocket<false, true, PerSocketData> *ws);
    void on_message(uWS::WebSocket<false, true, PerSocketData> *ws, const std::string_view &message,
                    uWS::OpCode opCode);
    void on_close(uWS::WebSocket<false, true, PerSocketData> *ws, const std::string_view &message,
                  int opCode);

    void send(std::string id, const std::string &message, uWS::OpCode opCode = uWS::OpCode::TEXT);
    void send_all(const std::string &message, uWS::OpCode opCode = uWS::OpCode::TEXT);
    void clear_users(boost::container::flat_set<std::string> users_to_clean);

    WebSocketServer(bool verbose, std::string log_folder,
                    moodycamel::BlockingReaderWriterQueue<WebSocketReceivedMessage> &receive_queue,
                    int max_messages_per_second, int max_users, struct uWS::Loop *loop);
    ~WebSocketServer();
};
