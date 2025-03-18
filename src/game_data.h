#pragma once
#include <string>
#include "peer_data.h"
#include "lobby_data.h"
#include "script_lua.h"
#include "script_angelscript.h"
#include <unordered_set>

struct GameData {
    std::string id;
    std::string entrypoint;
    long tick_rate = 0;
    std::unordered_map<std::string, PeerData> peers;
    std::unordered_map<std::string, int64_t> disconnected_peers;
    std::unordered_map<std::string, LobbyData> lobbies;
    std::unordered_set<std::string> lobby_listing_peers;
    std::unordered_set<std::string> enabled_callbacks;
    ScriptLua lua;
    //ScriptAngelScript angelscript;
    
    void close() {
        lua.close();
        //angelscript.close();
    }
    void open() {
        lua.open();
        //angelscript.open();
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
