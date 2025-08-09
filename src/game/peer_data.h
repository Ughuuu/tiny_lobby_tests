#pragma once
#include <boost/container/flat_map.hpp>
#include <string>
#include <unordered_map>

#include "../common/any_type.h"

struct PeerData {
    std::string id;
    int order_id = 0;
    int64_t last_message_time = 0;
    std::string game_id;
    std::string reconnection_token;
    std::string lobby_id;
    std::string platform;
    std::string platform_id;
    boost::container::flat_map<std::string, AnyElement> public_data;
    boost::container::flat_map<std::string, AnyElement> private_data;
    boost::container::flat_map<std::string, AnyElement> user_data;
    bool disconnected = false;
    bool ready = false;
    boost::container::flat_map<std::string, AnyElement> public_data_diff;
    bool public_data_dirty = false;
    boost::container::flat_map<std::string, AnyElement> private_data_diff;
    bool private_data_dirty = false;
    boost::container::flat_map<std::string, AnyElement> user_data_diff;
    bool user_data_dirty = false;

    void leave_lobby() {
        lobby_id = "";
        ready = false;
        order_id = 0;
        public_data = {};
        private_data = {};
        disconnected = false;
    }

    boost::container::flat_map<std::string, AnyElement> to_dict(bool include_private = false,
                                                                bool include_reconnection = false) {
        boost::container::flat_map<std::string, AnyElement> peer_dict;
        peer_dict["id"] = AnyElement{id};
        peer_dict["oi"] = AnyElement{order_id};
        if (include_reconnection) {
            peer_dict["rt"] = AnyElement{reconnection_token};
        }
        peer_dict["l"] = AnyElement{lobby_id};
        peer_dict["d"] = AnyElement{public_data};
        peer_dict["p"] = AnyElement{platform};
        if (include_private) {
            peer_dict["_d"] = AnyElement{private_data};
        }
        peer_dict["ud"] = AnyElement{user_data};
        peer_dict["dc"] = AnyElement{disconnected};
        peer_dict["r"] = AnyElement{ready};
        return peer_dict;
    }
};
