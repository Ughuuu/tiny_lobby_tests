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
    std::string max_players;
    std::unordered_set<std::string> peer_ids;
    std::time_t createTime;
    std::string game_id;
    bool sealed = false;
    std::unordered_map<std::string, AnyElement> public_data;
    std::unordered_map<std::string, AnyElement> private_data;
    std::unordered_map<std::string, AnyElement> tags;
    std::unordered_map<std::string, std::string> timer_data;
    int order_id_counter = 0;
};
