#pragma once
#include <atomic>
#include <string>
#include <unordered_map>

struct AnyType;

using VariantType = std::variant<int, std::string, std::unordered_map<std::string, AnyType>>;

struct AnyType {
    VariantType value;
};

struct PeerData {
    std::string id;
    int order_id = 0;
    std::string game_id;
    std::string reconnection_id;
    std::string lobby_id;
    std::unordered_map<std::string, AnyType> public_data;
    std::unordered_map<std::string, AnyType> private_data;
    std::unordered_map<std::string, AnyType> user_data;
    bool disconnected = false;
    bool ready = false;
};
