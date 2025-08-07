#pragma once
#include <readerwriterqueue.h>

#include <atomic>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/container/flat_map.hpp>
#include <iostream>
#include <string>

#include "../common/any_type.h"
#include "yyjson.h"

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

class LoginClient {
   public:
    LoginClient(const std::string& host, const std::string& game_id,
                const std::string& port = "443");

    bool connect(std::string& error);
    bool request_url(const std::string& type, std::string& url, std::string& login_type,
                     std::string& error);
    bool wait_for_jwt(std::string& jwt, std::string& type, std::string& access_token,
                      std::string& error);
    std::string verify_jwt(const std::string& jwt, std::string& error);
    void close();

   private:
    std::string host, port, game_id;
    net::io_context ioc;
    ssl::context ctx;
    tcp::resolver resolver;
    websocket::stream<ssl::stream<tcp::socket>> ws;
};
