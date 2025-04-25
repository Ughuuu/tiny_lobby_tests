#include "login_client.h"

#include "yyjson.h"

// Specialize the teardown for SSL stream
namespace boost::beast {

template <>
void teardown(beast::role_type role, ssl::stream<tcp::socket>& stream, beast::error_code& ec) {
    // Perform SSL shutdown
    stream.shutdown(ec);
    if (ec == net::error::eof) {
        ec = {};
    }
}

}  // namespace boost::beast

LoginClient::LoginClient(const std::string& host, const std::string& game_id,
                         const std::string& port)
    : host(host),
      game_id(game_id),
      port(port),
      ctx(ssl::context::tlsv12_client),
      resolver(ioc),
      ws(ioc, ctx) {}

bool LoginClient::connect(std::string& error) {
    try {
        ctx.set_verify_mode(ssl::verify_peer);

        auto results = resolver.resolve(host, port);
        auto ep = net::connect(beast::get_lowest_layer(ws), results);

        if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host.c_str())) {
            throw beast::system_error(static_cast<int>(::ERR_get_error()),
                                      net::error::get_ssl_category());
        }

        ws.next_layer().set_verify_callback(ssl::host_name_verification(host));

        std::string ws_host = host + ":" + std::to_string(ep.port());

        ws.next_layer().handshake(ssl::stream_base::client);

        ws.set_option(websocket::stream_base::decorator([this](websocket::request_type& req) {
            req.set(boost::beast::http::field::sec_websocket_protocol, "blazium," + game_id);
        }));

        ws.handshake(ws_host, "/api/v1/connect");
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

void LoginClient::close() {
    beast::error_code ec;
    ws.close(websocket::close_code::normal, ec);
    ws.next_layer().shutdown(ec);
}

std::string extract_login_url(const std::string& resp) {
    yyjson_doc* doc = yyjson_read(resp.c_str(), resp.size(), 0);
    if (!doc) return "";

    yyjson_val* root = yyjson_doc_get_root(doc);
    yyjson_val* url_val = yyjson_obj_get(root, "login_url");

    std::string result;
    if (url_val && yyjson_is_str(url_val)) {
        result = yyjson_get_str(url_val);
    }

    yyjson_doc_free(doc);
    return result;
}

bool LoginClient::request_url(const std::string& type, std::string& url, std::string& login_type,
                              std::string& error) {
    try {
        std::string msg = R"({"action":"getLogin","type":")" + type + R"("})";
        ws.write(net::buffer(msg));

        beast::flat_buffer buffer;
        ws.read(buffer);
        std::string resp = beast::buffers_to_string(buffer.data());

        auto url_pos = resp.find("\"login_url\":\"");
        if (url_pos != std::string::npos) {
            url = extract_login_url(resp);
            return true;
        } else {
            error = "Could not parse login_url response";
            return false;
        }
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

bool LoginClient::wait_for_jwt(std::string& jwt, std::string& type, std::string& access_token,
                               std::string& error) {
    try {
        beast::flat_buffer buffer;
        ws.read(buffer);
        std::string resp = beast::buffers_to_string(buffer.data());

        auto jwt_pos = resp.find("\"jwt\":\"");
        auto type_pos = resp.find("\"type\":\"");
        auto token_pos = resp.find("\"access_token\":\"");
        if (jwt_pos != std::string::npos && type_pos != std::string::npos &&
            token_pos != std::string::npos) {
            jwt_pos += 7;
            auto jwt_end = resp.find("\"", jwt_pos);
            jwt = resp.substr(jwt_pos, jwt_end - jwt_pos);

            type_pos += 8;
            auto type_end = resp.find("\"", type_pos);
            type = resp.substr(type_pos, type_end - type_pos);

            token_pos += 16;
            auto token_end = resp.find("\"", token_pos);
            access_token = resp.substr(token_pos, token_end - token_pos);
            return true;
        } else {
            error = "Could not parse jwt response";
            return false;
        }
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}
