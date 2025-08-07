#pragma once
#include <string>

struct PerSocketData {
    std::string uid;
    std::string id;
    std::string game_id;
    std::string reconnection_token;
    std::string platform = "anon";
    std::string name;
    std::string platform_id;
    int message_count = 0;
    int64_t last_message_time = 0;
};
