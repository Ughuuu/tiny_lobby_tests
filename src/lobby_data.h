#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "peer_data.h"
#include "any_type.h"

struct LobbyData {
    std::string id;
    std::string name;
    std::string host;
    std::string password;
    int max_players;
    std::set<std::string> peer_ids;
    int64_t create_time;
    std::string game_id;
    bool sealed = false;
    std::unordered_map<std::string, AnyElement> public_data;
    std::unordered_map<std::string, AnyElement> private_data;
    std::unordered_map<std::string, AnyElement> tags;
    int order_id_counter = 0;
    bool public_data_dirty = false;
    bool private_data_dirty = false;
    bool tags_dirty = false;
    bool sealed_dirty = false;

    std::string to_string(bool include_private = false) {
        std::unordered_map<std::string, AnyElement> lobby_dict;
        lobby_dict["id"] = AnyElement{id};
        lobby_dict["name"] = AnyElement{name};
        lobby_dict["host"] = AnyElement{host};
        lobby_dict["sealed"] = AnyElement{sealed};
        lobby_dict["max_players"] = AnyElement{max_players};
        lobby_dict["players"] = AnyElement{int(peer_ids.size())};
        lobby_dict["created_at"] = AnyElement{create_time};
        lobby_dict["has_password"] = AnyElement{bool(password != "")};
        lobby_dict["tags"] = AnyElement{tags};
        lobby_dict["public_data"] = AnyElement{public_data};
        if (include_private) {
            lobby_dict["private_data"] = AnyElement{private_data};
        }
        return AnyElement{lobby_dict}.to_string();
    }
};
