#pragma once
#include <boost/container/flat_map.hpp>
#include <string>
#include <unordered_map>

#include "any_type.h"

struct PeerData {
    std::string id;
    int order_id = 0;
    std::string game_id;
    std::string reconnection_token;
    std::string lobby_id;
    boost::container::flat_map<std::string, AnyElement> public_data;
    boost::container::flat_map<std::string, AnyElement> private_data;
    boost::container::flat_map<std::string, AnyElement> user_data;
    bool disconnected = false;
    bool ready = false;
    bool public_data_dirty = false;
    bool private_data_dirty = false;

    void leave_lobby() {
        lobby_id = "";
        ready = false;
        order_id = 0;
    }

    std::string to_string(bool include_private = false, bool include_reconnection = false) {
        boost::container::flat_map<std::string, AnyElement> peer_dict;
        peer_dict["id"] = AnyElement{id};
        peer_dict["order_id"] = AnyElement{order_id};
        if (include_reconnection) {
            peer_dict["reconnection_token"] = AnyElement{reconnection_token};
        }
        peer_dict["lobby_id"] = AnyElement{lobby_id};
        peer_dict["public_data"] = AnyElement{public_data};
        if (include_private) {
            peer_dict["private_data"] = AnyElement{private_data};
        }
        peer_dict["user_data"] = AnyElement{user_data};
        peer_dict["is_disconnected"] = AnyElement{disconnected};
        peer_dict["ready"] = AnyElement{ready};
        return AnyElement{peer_dict}.to_string();
    }
};
