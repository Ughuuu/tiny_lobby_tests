#pragma once
#include <readerwriterqueue.h>

#include <atomic>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/container/flat_map.hpp>
#include <iostream>
#include <string>

#include "any_type.h"
#include "yyjson.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

struct AnalyticsEvent {
    std::string event;
    boost::container::flat_map<std::string, AnyElement> event_data;
    std::string event_flag;
    std::string event_key;
    std::string event_type;
    std::string sub_event;
};

struct POGRClient {
    moodycamel::BlockingReaderWriterQueue<AnalyticsEvent> &analytics_queue;
    std::string client_id;
    std::string build_id;
    std::string access_key;
    std::string secret_key;
    std::string association_id;
    std::string pogr_url = "https://api.pogr.io";
    bool enabled = false;
    std::string session_id;
    std::atomic<bool> &stop;

    static std::string get_os_name() {
#if defined(_WIN32)
        return "Windows";
#elif defined(__APPLE__)
        return "macOS";
#elif defined(__linux__)
        return "Linux";
#else
        return "Unknown OS";
#endif
    }

    static std::string get_arch_name() {
#if defined(__x86_64__) || defined(_M_X64)
        return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
        return "arm64";
#elif defined(__i386__) || defined(_M_IX86)
        return "x86";
#elif defined(__arm__) || defined(_M_ARM)
        return "arm";
#else
        return "Unknown Arch";
#endif
    }

    void send_request(const std::string &target, const std::string &body, http::fields &headers) {
        try {
            auto pos = pogr_url.find("://");
            std::string host = pogr_url.substr(pos + 3);
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

            http::request<http::string_body> req{http::verb::post, target, 11};
            req.set(http::field::host, host);
            req.set(http::field::content_type, "application/json");
            req.body() = body;
            req.prepare_payload();

            for (const auto &header : headers) {
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
        } catch (const std::exception &e) {
            std::cerr << "Exception: " << e.what() << target << std::endl;
        }
    }

    void init() {
        if (client_id.empty() || build_id.empty()) return;

        http::fields headers;
        if (!access_key.empty() && !secret_key.empty()) {
            headers.set("ACCESS_KEY", access_key);
            headers.set("SECRET_KEY", secret_key);
        } else {
            headers.set("POGR_CLIENT", client_id);
            headers.set("POGR_BUILD", build_id);
        }

        std::string body = "{\"association_id\":\"" + association_id + "\"}";
        try {
            auto pos = pogr_url.find("://");
            std::string host = pogr_url.substr(pos + 3);
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

            http::request<http::string_body> req{http::verb::post, "/v1/intake/init", 11};
            req.set(http::field::host, host);
            req.set(http::field::content_type, "application/json");
            req.body() = body;
            req.prepare_payload();

            for (const auto &header : headers) {
                req.set(header.name_string(), header.value());
            }

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(stream, buffer, res);

            if (res.result() != http::status::ok) {
                std::cerr << "Error: " << res.result_int() << " " << res.reason() << std::endl;
            } else {
                std::string body = res.body();
                yyjson_doc *doc = yyjson_read(body.c_str(), body.length(), 0);
                if (!doc) {
                    std::cout << "Error sending init to pogr " << body << std::endl;
                    return;
                }
                yyjson_val *root = yyjson_doc_get_root(doc);
                if (!root || !yyjson_is_obj(root)) {
                    yyjson_doc_free(doc);
                    std::cout << "Error: root not found " << body << std::endl;
                    return;
                }
                yyjson_val *payload = yyjson_obj_get(root, "payload");
                if (!payload) {
                    yyjson_doc_free(doc);
                    std::cout << "Error: payload not found " << body << std::endl;
                    return;
                }
                yyjson_val *session_id_val = yyjson_obj_get(payload, "session_id");
                if (!session_id_val) {
                    yyjson_doc_free(doc);
                    std::cout << "Error: session_id not found " << body << std::endl;
                    return;
                }
                const char *session_id_str = yyjson_get_str(session_id_val);
                if (!session_id_str) {
                    yyjson_doc_free(doc);
                    std::cout << "Error: session_id not found " << body << std::endl;
                    return;
                }
                session_id = std::string(session_id_str);
                yyjson_doc_free(doc);
            }

            beast::error_code ec;
            stream.shutdown(ec);
        } catch (const std::exception &e) {
            std::cerr << "Exception: " << e.what() << "init" << std::endl;
        }
    }

    void data(const boost::container::flat_map<std::string, AnyElement> &data) {
        http::fields headers;
        headers.set("INTAKE_SESSION_ID", session_id);

        boost::container::flat_map<std::string, AnyElement> data_map;
        data_map["data"] = AnyElement{data};
        data_map["tags"] = AnyElement{boost::container::flat_map<std::string, AnyElement>{
            {"association_id", AnyElement{association_id}}}};

        send_request("/v1/intake/data", AnyElement{data_map}.to_string(), headers);
    }

    void event(const std::string &event,
               const boost::container::flat_map<std::string, AnyElement> &event_data,
               const std::string &event_flag, const std::string &event_key,
               const std::string &event_type, const std::string &sub_event) {
        http::fields headers;
        headers.set("INTAKE_SESSION_ID", session_id);

        boost::container::flat_map<std::string, AnyElement> data_map;
        data_map["event"] = AnyElement{event};
        data_map["event_data"] = AnyElement{event_data};
        data_map["event_flag"] = AnyElement{event_flag};
        data_map["event_key"] = AnyElement{event_key};
        data_map["event_type"] = AnyElement{event_type};
        data_map["sub_event"] = AnyElement{sub_event};
        data_map["tags"] = AnyElement{boost::container::flat_map<std::string, AnyElement>{
            {"association_id", AnyElement{association_id}}}};

        send_request("/v1/intake/event", AnyElement{data_map}.to_string(), headers);
    }

    void run() {
        while (!stop) {
            AnalyticsEvent analytics_event;
            analytics_queue.wait_dequeue(analytics_event);
            event(analytics_event.event, analytics_event.event_data, analytics_event.event_flag,
                  analytics_event.event_key, analytics_event.event_type, analytics_event.sub_event);
        }
    }
};
