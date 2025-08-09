#pragma once
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "../common/any_type.h"
#include "../lua/script_lua.h"
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
    ScriptLua lua;

    boost::container::flat_map<std::string, AnyElement> to_dict(bool include_private = false) {
        boost::container::flat_map<std::string, AnyElement> lobby_dict;
        lobby_dict["id"] = AnyElement{id};
        lobby_dict["n"] = AnyElement{name};
        lobby_dict["h"] = AnyElement{host};
        lobby_dict["s"] = AnyElement{sealed};
        lobby_dict["m"] = AnyElement{max_players};
        lobby_dict["p"] = AnyElement{int(peer_ids.size())};
        lobby_dict["c"] = AnyElement{create_time};
        lobby_dict["_p"] = AnyElement{bool(password != "")};
        lobby_dict["t"] = AnyElement{tags};
        lobby_dict["p"] = AnyElement{public_data};
        if (include_private) {
            lobby_dict["_p"] = AnyElement{private_data};
        }
        return lobby_dict;
    }

    void close() {
        if (lua.enabled) {
            lua.close();
        }
    }
    void open() {
        if (lua.enabled) {
            lua.open();
        }
    }
};
