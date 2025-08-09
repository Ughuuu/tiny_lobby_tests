#pragma once
#include <readerwriterqueue.h>
#include <uwebsockets/App.h>

#include <memory>
#include <string>

#include "../common/server_logger.h"
#include "../database/database.h"
#include "../websocket/per_socket_data.h"

struct AuthenticationMessage {
    PerSocketData user_data;
    uWS::HttpResponse<false> *res;
    uWS::HttpRequest *req;
    struct us_socket_context_t *context;
    std::shared_ptr<bool> abort;
    std::string websocket_key;
    std::string websocket_extensions;
};

class AuthenticationThread {
    moodycamel::BlockingReaderWriterQueue<AuthenticationMessage> &authentication_queue;
    ServerLogger logger;
    struct uWS::Loop *loop;
    Database db;
    bool running{true};

   public:
    void run();
    void stop();
    AuthenticationThread(
        moodycamel::BlockingReaderWriterQueue<AuthenticationMessage> &authentication_queue,
        bool verbose, std::string log_folder, struct uWS::Loop *loop);
};
