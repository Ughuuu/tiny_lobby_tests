#pragma once
#include <readerwriterqueue.h>

#include <boost/container/flat_map.hpp>

#include "any_type.h"
#include "httplib.h"
#include "yyjson.h"
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
    void init() {
        if (client_id.empty() || build_id.empty()) {
            return;
        }
        httplib::Client client(pogr_url);
        httplib::Headers headers;
        if (!access_key.empty() && !secret_key.empty()) {
            headers.insert({"ACCESS_KEY", access_key});
            headers.insert({"SECRET_KEY", secret_key});
        } else {
            headers.insert({"POGR_CLIENT", client_id});
            headers.insert({"POGR_BUILD", build_id});
        }
        client.set_default_headers(headers);
        httplib::Result result =
            client.Post("/v1/intake/init", "{\"association_id\":\"" + association_id + "\"}",
                        "application/json");
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
        }
    }
    void data(boost::container::flat_map<std::string, AnyElement> data) {
        httplib::Client client(pogr_url);
        httplib::Headers headers;
        headers.insert({"INTAKE_SESSION_ID", session_id});
        boost::container::flat_map<std::string, AnyElement> data_map;
        data_map["data"] = AnyElement{data};
        data_map["tags"] = AnyElement{boost::container::flat_map<std::string, AnyElement>{
            {"association_id", AnyElement{association_id}}}};
        client.set_default_headers(headers);
        httplib::Result result =
            client.Post("/v1/intake/data", AnyElement{data_map}.to_string(), "application/json");
        if (result.error() != httplib::Error::Success) {
            std::cout << "Error: " << result.error() << std::endl;
        }
    }
    void event(std::string event, boost::container::flat_map<std::string, AnyElement> event_data,
               std::string event_flag, std::string event_key, std::string event_type,
               std::string sub_event) {
        httplib::Client client(pogr_url);
        httplib::Headers headers;
        headers.insert({"INTAKE_SESSION_ID", session_id});
        boost::container::flat_map<std::string, AnyElement> data_map;
        data_map["event"] = AnyElement{event};
        data_map["event_data"] = AnyElement{event_data};
        data_map["event_flag"] = AnyElement{event_flag};
        data_map["event_key"] = AnyElement{event_key};
        data_map["event_type"] = AnyElement{event_type};
        data_map["sub_event"] = AnyElement{sub_event};
        data_map["tags"] = AnyElement{boost::container::flat_map<std::string, AnyElement>{
            {"association_id", AnyElement{association_id}}}};
        client.set_default_headers(headers);
        httplib::Result result =
            client.Post("/v1/intake/event", AnyElement{data_map}.to_string(), "application/json");
        if (result.error() != httplib::Error::Success) {
            std::cout << "Error: " << result.error() << std::endl;
        }
    }
    void run() {
        while (true) {
            AnalyticsEvent analytics_event;
            analytics_queue.wait_dequeue(analytics_event);
            event(analytics_event.event, analytics_event.event_data, analytics_event.event_flag,
                  analytics_event.event_key, analytics_event.event_type, analytics_event.sub_event);
        }
    }
};
