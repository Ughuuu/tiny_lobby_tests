#pragma once
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <readerwriterqueue.h>
#include "App.h"
#include "server_logger.h"

struct PerSocketData {
    std::string id;
    std::string game_id;
    std::string reconnection_id;
};

enum WebSocketEvent {
    OPEN,
    MESSAGE,
    CLOSE
};

struct WebSocketMessage {
    std::string id;
    WebSocketEvent event;
    std::string message;
    std::string game_id;
    std::string reconnection_id;
};

template <bool SSL>
class WebSocketServer {
    boost::uuids::random_generator gen;
    moodycamel::BlockingReaderWriterQueue<WebSocketMessage> &message_queue;
    ServerLogger logger;
    std::unordered_map<std::string, uWS::WebSocket<SSL, true, PerSocketData>*> sockets;
public:
    void on_upgrade(uWS::HttpResponse<SSL> *res, uWS::HttpRequest *req, struct us_socket_context_t *context);
    void on_open(uWS::WebSocket<SSL, true, PerSocketData> *ws);
    void on_message(uWS::WebSocket<SSL, true, PerSocketData> *ws, const std::string_view &message, uWS::OpCode opCode);
    void on_close(uWS::WebSocket<SSL, true, PerSocketData> *ws, const std::string_view &message, int opCode);
    void send(std::string id, const std::string &message, uWS::OpCode opCode = uWS::OpCode::TEXT);

    WebSocketServer(bool verbose, moodycamel::BlockingReaderWriterQueue<WebSocketMessage>& message_queue);
};

#include "websocket_server.tpp"
