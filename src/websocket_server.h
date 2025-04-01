#pragma once
#include <readerwriterqueue.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/container/flat_set.hpp>

#include "App.h"
#include "server_logger.h"

static int64_t get_time_now();

struct PerSocketData {
    std::string uid;
    std::string id;
    std::string game_id;
    std::string reconnection_token;
    int message_count = 0;
    int64_t last_message_time = 0;
};

template <bool SSL>
struct PeerConnectionData {
    std::string id;
    std::string game_id;
    std::string reconnection_token;
    uWS::WebSocket<SSL, true, PerSocketData> *ws;
};

enum WebSocketEvent { OPEN, MESSAGE, CLOSE };

struct WebSocketReceivedMessage {
    std::string id;
    WebSocketEvent event;
    std::string message;
    std::string game_id;
    std::string reconnection_token;
};

struct ReconnectionTokens {
    std::string peer_id;
    int64_t timestamp;
};

template <bool SSL>
class WebSocketServer {
    int MAX_RECONNECTION_TIME = 6 * 60 * 1000;
    int max_users = 100000;
    int max_messages_per_second;
    int connected_users = 0;
    boost::uuids::random_generator gen;
    moodycamel::BlockingReaderWriterQueue<WebSocketReceivedMessage> &receive_queue;
    ServerLogger logger;
    std::unordered_map<std::string, PeerConnectionData<SSL>> connection_data;
    std::unordered_map<std::string, ReconnectionTokens> reconnections;

   public:
    void tick();
    void on_upgrade(uWS::HttpResponse<SSL> *res, uWS::HttpRequest *req,
                    struct us_socket_context_t *context);
    void on_open(uWS::WebSocket<SSL, true, PerSocketData> *ws);
    void on_message(uWS::WebSocket<SSL, true, PerSocketData> *ws, const std::string_view &message,
                    uWS::OpCode opCode);
    void on_close(uWS::WebSocket<SSL, true, PerSocketData> *ws, const std::string_view &message,
                  int opCode);
                
    void send(std::string id, const std::string &message, uWS::OpCode opCode = uWS::OpCode::TEXT);
    void clear_users(boost::container::flat_set<std::string> users_to_clean);

/*************  ✨ Codeium Command ⭐  *************/
    /**
     * @brief Construct a new WebSocketServer object
     *
     * @param verbose Verbosity flag for logging
     * @param log_folder Folder where logs will be stored
     * @param receive_queue Queue where received messages will be stored
     * @param max_messages_per_second Maximum number of messages per second
     */
/******  06c90573-aca3-4305-99d6-2cad47e55444  *******/
    WebSocketServer(bool verbose, std::string log_folder,
                    moodycamel::BlockingReaderWriterQueue<WebSocketReceivedMessage> &receive_queue,
                    int max_messages_per_second);
};

#include "websocket_server.tpp"
