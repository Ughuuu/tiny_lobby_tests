#include "script_as_http.h"

#include <scriptarray/scriptarray.h>

#include <boost/asio/connect.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/version.hpp>
#include <sstream>

#include "script_as.h"
#include "script_as_functions.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

http::response<http::string_body> request(
    const std::string &method, const std::string &url,
    const std::vector<std::pair<std::string, std::string>> &query_params,
    const std::vector<std::pair<std::string, std::string>> &headers, const std::string &body) {
    std::string scheme, host, target;
    int port = 443;
    if (url.find("http://") == 0) {
        scheme = "http";
        port = 80;
    } else if (url.find("https://") == 0) {
        scheme = "https";
    } else {
        throw std::runtime_error("Invalid URL scheme");
    }

    auto pos = url.find("://");
    auto remainder = url.substr(pos + 3);
    auto slash = remainder.find("/");
    host = remainder.substr(0, slash);
    target = slash != std::string::npos ? remainder.substr(slash) : "/";

    std::ostringstream query_stream;
    for (size_t i = 0; i < query_params.size(); ++i) {
        if (i > 0) query_stream << "&";
        query_stream << query_params[i].first << "=" << query_params[i].second;
    }
    std::string query_str = query_stream.str();
    if (!query_str.empty()) {
        target += "?" + query_str;
    }

    net::io_context ioc;
    ssl::context ctx(ssl::context::tlsv12_client);
    ctx.set_default_verify_paths();

    tcp::resolver resolver(ioc);
    auto const results = resolver.resolve(host, std::to_string(port));

    beast::tcp_stream tcp_stream(ioc);
    tcp_stream.connect(results);

    ssl::stream<beast::tcp_stream> stream(std::move(tcp_stream), ctx);
    if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
        throw beast::system_error(
            beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()));
    }
    stream.handshake(ssl::stream_base::client);

    http::request<http::string_body> req;
    req.version(11);
    req.method(http::string_to_verb(method));
    req.target(target);
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    for (const auto &h : headers) {
        req.set(h.first, h.second);
    }
    req.body() = body;
    req.prepare_payload();

    http::write(stream, req);

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    beast::error_code ec;
    stream.shutdown(ec);
    return res;
}

ScriptASHttpResponse *http_request_as(const std::string &method, const std::string &url,
                                      CScriptArray *query_params_arr, CScriptArray *headers_arr,
                                      const std::string &body, asIScriptEngine *engine) {
    std::vector<std::pair<std::string, std::string>> query_params;
    std::vector<std::pair<std::string, std::string>> headers;
    auto query_params_any = ConvertFromArray(engine, query_params_arr);
    auto headers_any = ConvertFromArray(engine, headers_arr);
    for (size_t i = 0; i + 1 < query_params_any.size(); i += 2) {
        if (auto key = std::get_if<std::string>(&query_params_any[i].value)) {
            if (auto val = std::get_if<std::string>(&query_params_any[i + 1].value)) {
                query_params.emplace_back(*key, *val);
            }
        }
    }
    for (size_t i = 0; i + 1 < headers_any.size(); i += 2) {
        if (auto key = std::get_if<std::string>(&query_params_any[i].value)) {
            if (auto val = std::get_if<std::string>(&query_params_any[i + 1].value)) {
                headers.emplace_back(*key, *val);
            }
        }
    }
    query_params_arr->Release();
    headers_arr->Release();
    try {
        auto res = request(method, url, query_params, headers, body);
        return new ScriptASHttpResponse(static_cast<int>(res.result_int()), res.body());
    } catch (const std::exception &e) {
        return new ScriptASHttpResponse(500, std::string("Error: ") + e.what());
    }
}

void http_request_as_wrapper(asIScriptGeneric *gen) {
    auto method = *static_cast<std::string *>(gen->GetArgObject(0));
    auto url = *static_cast<std::string *>(gen->GetArgObject(1));
    auto query_params_arr = static_cast<CScriptArray *>(gen->GetArgObject(2));
    auto headers_arr = static_cast<CScriptArray *>(gen->GetArgObject(3));
    auto body = *static_cast<std::string *>(gen->GetArgObject(4));

    ScriptASHttpResponse *res =
        http_request_as(method, url, query_params_arr, headers_arr, body, gen->GetEngine());
    gen->SetReturnObject(res);
}

void RegisterHTTPInterface(asIScriptEngine *engine) {
    int r;
    r = engine->RegisterObjectType("HttpResponse", 0, asOBJ_REF);
    assert(r >= 0);
    r = engine->RegisterObjectBehaviour("HttpResponse", asBEHAVE_ADDREF, "void f()",
                                        asMETHOD(ScriptASHttpResponse, AddRef), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectBehaviour("HttpResponse", asBEHAVE_RELEASE, "void f()",
                                        asMETHOD(ScriptASHttpResponse, Release), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("HttpResponse", "int get_status() const property",
                                     asMETHOD(ScriptASHttpResponse, get_status), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("HttpResponse", "string get_body() const property",
                                     asMETHOD(ScriptASHttpResponse, get_body), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->SetDefaultNamespace("Http");
    assert(r >= 0);
    r = engine->RegisterGlobalFunction(
        "HttpResponse@ request(const string &in method, const string &in url, const array<any>@ "
        "query_params = array<any>(), const array<any>@ headers = array<any>(), const string &in "
        "body = "
        ")",
        asFUNCTION(http_request_as_wrapper), asCALL_GENERIC);
    assert(r >= 0);
    r = engine->SetDefaultNamespace("");
    assert(r >= 0);
}
