#pragma once
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <readerwriterqueue.h>
#include "App.h"
#include "server_logger.h"

#define USES_SSL false

struct PerSocketData {
    boost::uuids::uuid id;
};

enum WebSocketEvent {
    OPEN,
    MESSAGE,
    CLOSE
};

struct WebSocketMessage {
    boost::uuids::uuid id;
    WebSocketEvent event;
    std::string message;
};

class WebSocketServer {
    boost::uuids::random_generator gen;
    moodycamel::BlockingReaderWriterQueue<WebSocketMessage> &message_queue;
    ServerLogger logger;
    std::unordered_map<boost::uuids::uuid, uWS::WebSocket<USES_SSL, true, PerSocketData>*> sockets;
public:
    void on_open(uWS::WebSocket<USES_SSL, true, PerSocketData> *ws);
    void on_message(uWS::WebSocket<USES_SSL, true, PerSocketData> *ws, const std::string_view &message, uWS::OpCode opCode);
    void on_close(uWS::WebSocket<USES_SSL, true, PerSocketData> *ws, const std::string_view &message, int opCode);
    void send(boost::uuids::uuid id, const std::string &message, uWS::OpCode opCode = uWS::OpCode::TEXT);

    WebSocketServer(bool verbose, moodycamel::BlockingReaderWriterQueue<WebSocketMessage>& message_queue);
};
