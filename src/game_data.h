#pragma once
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/vector.hpp>
#include <string>
#include <unordered_set>

#include "lobby_data.h"
#include "peer_data.h"
#include "script_angelscript.h"
#include "script_lua.h"

struct TimerData {
    std::string id;
    std::string lobby_id;
    std::string game_id;
    std::string peer_id;
    int64_t end_time;
    boost::container::vector<AnyElement> args;
};

struct GameData {
    int64_t last_send_time = 0;
    int64_t last_tick_time = 0;
    std::string id;
    std::string entrypoint;
    std::string lobby_control = "peer";
    int tick_rate = 0;
    int send_rate = 0;
    boost::container::flat_map<std::string, boost::container::vector<std::string>> peers_send_data;
    boost::container::flat_map<std::string, PeerData> peers;
    boost::container::flat_map<std::string, int64_t> disconnected_peers;
    boost::container::flat_map<std::string, LobbyData> lobbies;
    boost::container::flat_set<std::string> lobby_listing_peers;
    boost::container::flat_set<std::string> lobbies_updated;
    boost::container::flat_set<std::string> enabled_callbacks;
    boost::container::flat_map<std::string, TimerData> timer_data;
    ScriptLua lua;
    // ScriptAngelScript angelscript;

    void close() {
        lua.close();
        // angelscript.close();
    }
    void open() {
        lua.open();
        // angelscript.open();
    }

    std::string peers_to_string(std::string &lobby_id) {
        std::string result = "[";
        for (auto &peer_id : lobbies[lobby_id].peer_ids) {
            auto &peer = peers[peer_id];
            result += peer.to_string() + ",";
        }
        return result.substr(0, result.size() - 1) + "]";
    }
};
