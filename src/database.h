#pragma once
#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <tuple>
#include <vector>

class Database {
   public:
    Database();
    ~Database();
    bool ensure_connection();
    void connect_to_db();
    void close_connection();
    void leaderboard_set_score(const std::string& leaderboard_id, const std::string& game_id,
                               const std::string& user_id, int64_t score, std::string& mode);
    std::vector<std::tuple<std::string, int64_t, std::string>> leaderboard_get_top(
        const std::string& leaderboard_id, const std::string& game_id, int limit);
    std::pair<int64_t, int> leaderboard_get_user_score(const std::string& leaderboard_id,
                                                       const std::string& game_id,
                                                       const std::string& user_id);

   private:
    pqxx::connection* connection;
};
