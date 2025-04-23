#pragma once

#include <angelscript.h>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/container/flat_map.hpp>
#include <string>

#include "any_type.h"

class ScriptASHttpResponse {
   public:
    ScriptASHttpResponse(int status, const std::string &body)
        : status_code(status), body_str(body) {}
    int ref_count = 1;

    void AddRef() { ++ref_count; }
    void Release() {
        if (--ref_count == 0) {
            delete this;
        }
    }

    int get_status() const { return status_code; }
    std::string get_body() const { return body_str; }

   private:
    int status_code;
    std::string body_str;
};

boost::beast::http::response<boost::beast::http::string_body> request(
    const std::string &method, const std::string &url,
    const std::vector<std::pair<std::string, std::string>> &query_params,
    const std::vector<std::pair<std::string, std::string>> &headers, const std::string &body);
void RegisterHTTPInterface(asIScriptEngine *engine);
