#pragma once
#include "httplib.h"
#include "yyjson.h"
struct POGRClient {
    std::string client_id;
    std::string build_id;
    std::string pogr_url = "https://api.pogr.io";
    bool enabled = false;
    std::string session_id;
    void init() {
        if (client_id.empty() || build_id.empty()) {
            return;
        }
        httplib::Client client(pogr_url);
        client.set_default_headers({{"POGR_CLIENT", client_id}, {"POGR_BUILD", build_id}});
        httplib::Result result = client.Post("/v1/intake/init");
        if (result.error() != httplib::Error::Success) {
            std::cout << "Error: " << result.error() << std::endl;
        } else {
            std::string body = result->body;
            // decode as json
            yyjson_doc *doc = yyjson_read(body.c_str(), body.length(), 0);
            if (!doc) {
                std::cout << "Error sending init to pogr " << body << std::endl;
                return;
            }
            yyjson_val *root = yyjson_doc_get_root(doc);
            if (!root || !yyjson_is_obj(root)) {
                yyjson_doc_free(doc);
                std::cout << "Error: root not found "  << body << std::endl;
                return;
            }
            yyjson_val *payload = yyjson_obj_get(root, "payload");
            if (!payload) {
                yyjson_doc_free(doc);
                std::cout << "Error: payload not found "  << body << std::endl;
                return;
            }
            yyjson_val *session_id_val = yyjson_obj_get(payload, "session_id");
            if (!session_id_val) {
                yyjson_doc_free(doc);
                std::cout << "Error: session_id not found "  << body << std::endl;
                return;
            }
            const char *session_id_str = yyjson_get_str(session_id_val);
            if (!session_id_str) {
                yyjson_doc_free(doc);
                std::cout << "Error: session_id not found "  << body << std::endl;
                return;
            }
            session_id = std::string(session_id_str);
        }
    }
};
