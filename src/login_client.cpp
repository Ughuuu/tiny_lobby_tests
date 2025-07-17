#include "login_client.h"

#include "yyjson.h"

namespace http = boost::beast::http;

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

static std::string send_request(const boost::beast::http::verb& verb, const std::string& url,
                                const std::string& target, const std::string& body,
                                http::fields& headers) {
    try {
        auto pos = url.find("://");
        std::string host = url.substr(pos + 3);
        std::string port = "443";
        auto slash = host.find('/');
        if (slash != std::string::npos) host = host.substr(0, slash);

        net::io_context ioc;
        ssl::context ctx(ssl::context::tlsv12_client);
        ctx.set_default_verify_paths();

        tcp::resolver resolver(ioc);
        auto const results = resolver.resolve(host, port);

        beast::tcp_stream tcp_stream(ioc);
        tcp_stream.connect(results);

        ssl::stream<beast::tcp_stream> stream(std::move(tcp_stream), ctx);
        if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
            throw beast::system_error(beast::error_code(static_cast<int>(::ERR_get_error()),
                                                        net::error::get_ssl_category()),
                                      "Failed to set SNI Hostname");
        }
        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{verb, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();

        for (const auto& header : headers) {
            req.set(header.name_string(), header.value());
        }

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        if (res.result() != http::status::ok) {
            std::cerr << "Error: " << res.result_int() << " " << res.reason() << std::endl;
        } else {
            // Optionally handle response
        }

        beast::error_code ec;
        stream.shutdown(ec);
        return res.body();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << target << std::endl;
    }
    return "";
}

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
            req.set(boost::beast::http::field::sec_websocket_protocol, "appsinacup," + game_id);
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

std::string LoginClient::verify_jwt(const std::string& jwt, std::string& error) {
    http::fields headers;
    headers.set("SESSION", jwt);
    auto result = send_request(boost::beast::http::verb::get, "https://login.appsinacup.app",
                               "/api/v1/internal/token/verify", "", headers);
    yyjson_doc* doc = yyjson_read(result.c_str(), result.size(), 0);
    if (!doc) {
        return result;
    }
    // temporary hack, check steam ticket also
    yyjson_val* root = yyjson_doc_get_root(doc);
    if (!root) {
        return result;
    }
    boost::container::flat_map<std::string, AnyElement> auth_result;
    std::string decode_error = decode_object(root, auth_result);
    yyjson_doc_free(doc);
    if (!decode_error.empty()) {
        return result;
    }
    if (auth_result.find("success") == auth_result.end()) {
        return result;
    }
    auto& sucess_value = auth_result["success"];
    if (std::holds_alternative<bool>(sucess_value.value)) {
        bool success = std::get<bool>(sucess_value.value);
        if (!success) {
            // try with steam ticket
            auto steam_result =
                send_request(boost::beast::http::verb::get, "https://login.appsinacup.app",
                             "/api/v1/internal/steam/verify", "", headers);
            return steam_result;
        }
    } else {
        return result;
    }
    return result;
}
