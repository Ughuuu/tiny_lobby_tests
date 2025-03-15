#pragma once
#include <string>
#include <unordered_map>
#include "any_type.h"

struct PeerData {
    std::string id;
    int order_id = 0;
    std::string game_id;
    std::string reconnection_id;
    std::string lobby_id;
    std::unordered_map<std::string, AnyElement> public_data;
    std::unordered_map<std::string, AnyElement> private_data;
    std::unordered_map<std::string, AnyElement> user_data;
    bool disconnected = false;
    bool ready = false;
};
