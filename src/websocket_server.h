#pragma once
#include <readerwriterqueue.h>
#include <uwebsockets/App.h>

#include <boost/container/flat_set.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "authentication_thread.h"
#include "database.h"
#include "per_socket_data.h"
#include "server_logger.h"

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
