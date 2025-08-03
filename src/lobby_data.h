#pragma once
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "any_type.h"
#include "peer_data.h"

struct LobbyData {
    std::string id;
    std::string name;
    std::string host;
    std::string password;
    int64_t max_players;
    boost::container::flat_set<std::string> peer_ids;
    boost::container::vector<std::string> peer_ordered_ids;
    int64_t create_time;
    std::string game_id;
    bool sealed = false;
    bool disband_on_leave = false;
    boost::container::flat_map<std::string, AnyElement> public_data;
    boost::container::flat_map<std::string, AnyElement> private_data;
    boost::container::flat_map<std::string, AnyElement> tags;
    int64_t order_id_counter = 0;
    boost::container::flat_map<std::string, AnyElement> public_data_diff;
    bool public_data_dirty = false;
    boost::container::flat_map<std::string, AnyElement> private_data_diff;
    bool private_data_dirty = false;
    boost::container::flat_map<std::string, AnyElement> tags_diff;
    bool tags_dirty = false;
    bool max_players_dirty = false;
    bool sealed_dirty = false;
    bool name_dirty = false;

    boost::container::flat_map<std::string, AnyElement> to_dict(bool include_private = false) {
        boost::container::flat_map<std::string, AnyElement> lobby_dict;
        lobby_dict["id"] = AnyElement{id};
        lobby_dict["name"] = AnyElement{name};
        lobby_dict["host"] = AnyElement{host};
        lobby_dict["sealed"] = AnyElement{sealed};
        lobby_dict["m"] = AnyElement{max_players};
        lobby_dict["players"] = AnyElement{int(peer_ids.size())};
        lobby_dict["created_at"] = AnyElement{create_time};
        lobby_dict["has_password"] = AnyElement{bool(password != "")};
        lobby_dict["tags"] = AnyElement{tags};
        lobby_dict["public_data"] = AnyElement{public_data};
        if (include_private) {
            lobby_dict["private_data"] = AnyElement{private_data};
        }
        return lobby_dict;
    }
};
