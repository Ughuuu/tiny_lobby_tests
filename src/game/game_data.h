#pragma once
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/vector.hpp>
#include <string>
#include <unordered_set>

#include "lobby_data.h"
#include "peer_data.h"
#include "../lua/script_lua.h"

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
    std::string lobby_control = "relay";
    int tick_rate = 0;
    int send_rate = 0;
    int max_afk_time = 0;
    bool seal = false;
    bool disband_on_leave = false;
    std::string folder_name;
    boost::container::flat_map<std::string, boost::container::vector<std::string>> peers_send_data;
    boost::container::flat_map<std::string, PeerData> peers;
    boost::container::flat_map<std::string, int64_t> disconnected_peers;
    std::unordered_map<std::string, LobbyData> lobbies;
    boost::container::flat_set<std::string> lobby_listing_peers;
    boost::container::flat_set<std::string> lobbies_updated;
    boost::container::flat_map<std::string, TimerData> timer_data;
    boost::container::flat_set<std::string> enabled_callbacks;
    ScriptLua lua;

    boost::container::vector<AnyElement> peers_to_array(std::string &lobby_id) {
        boost::container::vector<AnyElement> result;
        for (auto &peer_id : lobbies[lobby_id].peer_ids) {
            auto &peer = peers[peer_id];
            result.push_back(AnyElement{peer.to_dict()});
        }
        return result;
    }

    void close() {
        if (lua.enabled) {
            lua.close();
        }
    }
    void open(int64_t now) {
        if (lua.enabled) {
            enabled_callbacks = lua.open();
        }
        // set last_send_time and last_tick_time to multiple of send_rate and tick_rate
        if (send_rate > 0) {
            last_send_time = (now / send_rate) * send_rate;
        }
        if (tick_rate > 0) {
            last_tick_time = (now / tick_rate) * tick_rate;
        }
    }
};
