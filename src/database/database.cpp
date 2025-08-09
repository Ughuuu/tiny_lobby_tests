#include "database.h"

#include <iostream>
#include <pqxx/pqxx>
#include <tuple>
#include <vector>

#include "../common/base_path.h"
#include "INIReader.h"

Database::Database() : connection(nullptr) {
    INIReader config_reader(BasePath::instance().file("config.ini"));
    enabled = config_reader.GetBoolean("database", "enabled", false);
}

Database::~Database() { close_connection(); }

bool Database::ensure_connection() {
    if (!connection || !connection->is_open()) {
        std::cerr << "Connection lost, attempting to reconnect..." << std::endl;
        connect_to_db();
        return false;
    }
    return true;
}

void Database::connect_to_db() {
    try {
        INIReader config_reader(BasePath::instance().file("config.ini"));
        std::string database = config_reader.Get("database", "database", "");
        if (database.empty()) {
            std::cerr << "Database name not found in config.ini" << std::endl;
            exit(1);
        }
        std::string user = config_reader.Get("database", "user", "");
        if (user.empty()) {
            std::cerr << "Database user not found in config.ini" << std::endl;
            exit(1);
        }
        std::string password = config_reader.Get("database", "password", "");
        if (password.empty()) {
            std::cerr << "Database password not found in config.ini" << std::endl;
            exit(1);
        }
        std::string host = config_reader.Get("database", "host", "");
        if (host.empty()) {
            std::cerr << "Database host not found in config.ini" << std::endl;
            exit(1);
        }
        std::string port = config_reader.Get("database", "port", "");
        if (port.empty()) {
            std::cerr << "Database port not found in config.ini" << std::endl;
            exit(1);
        }
        std::string conn_str = "dbname=" + database + " user=" + user + " password=" + password +
                               " host=" + host + " port=" + port;
        connection = new pqxx::connection(conn_str);
        if (connection->is_open()) {
            std::cout << "Connected to database: " << connection->dbname() << std::endl;
        } else {
            std::cerr << "Can't open database" << std::endl;
            exit(1);
        }
        pqxx::work txn(*connection);
        txn.exec(
            "CREATE TABLE IF NOT EXISTS leaderboards ("
            "id SERIAL PRIMARY KEY, "
            "leaderboard_id TEXT NOT NULL, "
            "game_id TEXT NOT NULL, "
            "user_id TEXT NOT NULL, "
            "score BIGINT NOT NULL, "
            "timestamp TIMESTAMPTZ DEFAULT NOW(), "
            "UNIQUE (leaderboard_id, game_id, user_id)"
            ");");
        txn.exec(
            "CREATE INDEX IF NOT EXISTS idx_leaderboard_game_score ON leaderboards "
            "(leaderboard_id, "
            "game_id, score DESC);");
        txn.exec(
            "CREATE TABLE IF NOT EXISTS peers ("
            "id TEXT NOT NULL, "
            "peer_id TEXT NOT NULL, "
            "game_id TEXT NOT NULL, "
            "timestamp TIMESTAMPTZ DEFAULT NOW(), "
            "PRIMARY KEY (id, game_id)"
            ");");
        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
}

void Database::close_connection() {
    if (connection) {
        connection->close();
        delete connection;
        connection = nullptr;
    }
}

// Set or update a user's score with mode
void Database::leaderboard_set_score(const std::string& leaderboard_id, const std::string& game_id,
                                     const std::string& user_id, int64_t score, std::string& mode) {
    ensure_connection();
    pqxx::work txn(*connection);
    if (mode == "set") {
        txn.exec(pqxx::zview("INSERT INTO leaderboards (leaderboard_id, game_id, user_id, score) "
                             "VALUES ($1, $2, $3, "
                             "$4) "
                             "ON CONFLICT (leaderboard_id, game_id, user_id) DO UPDATE SET score = "
                             "$4, timestamp = "
                             "NOW();"),
                 pqxx::params(leaderboard_id, game_id, user_id, score));
    } else if (mode == "best") {
        txn.exec(pqxx::zview("INSERT INTO leaderboards (leaderboard_id, game_id, user_id, score) "
                             "VALUES ($1, $2, $3, "
                             "$4) "
                             "ON CONFLICT (leaderboard_id, game_id, user_id) DO UPDATE SET score = "
                             "GREATEST(leaderboards.score, EXCLUDED.score), timestamp = NOW();"),
                 pqxx::params(leaderboard_id, game_id, user_id, score));
    } else if (mode == "inc") {
        txn.exec(pqxx::zview("INSERT INTO leaderboards (leaderboard_id, game_id, user_id, score) "
                             "VALUES ($1, $2, $3, "
                             "$4) "
                             "ON CONFLICT (leaderboard_id, game_id, user_id) DO UPDATE SET score = "
                             "leaderboards.score + EXCLUDED.score, timestamp = NOW();"),
                 pqxx::params(leaderboard_id, game_id, user_id, score));
    } else if (mode == "dec") {
        txn.exec(pqxx::zview("INSERT INTO leaderboards (leaderboard_id, game_id, user_id, score) "
                             "VALUES ($1, $2, $3, "
                             "$4) "
                             "ON CONFLICT (leaderboard_id, game_id, user_id) DO UPDATE SET score = "
                             "leaderboards.score - EXCLUDED.score, timestamp = NOW();"),
                 pqxx::params(leaderboard_id, game_id, user_id, score));
    }
    txn.commit();
}

// Get top N scores for a game
std::vector<std::tuple<std::string, int64_t, std::string>> Database::leaderboard_get_top(
    const std::string& leaderboard_id, const std::string& game_id, int limit, int start) {
    ensure_connection();
    pqxx::work txn(*connection);
    pqxx::result r =
        txn.exec(pqxx::zview("SELECT user_id, score, to_char(timestamp, 'YYYY-MM-DDHH24:MI:SS') "
                             "FROM leaderboards WHERE leaderboard_id = $1 AND game_id = $2 "
                             "ORDER BY score DESC LIMIT $3 OFFSET $4;"),
                 pqxx::params(leaderboard_id, game_id, limit, start));
    std::vector<std::tuple<std::string, int64_t, std::string>> results;
    for (auto row : r) {
        results.emplace_back(row[0].as<std::string>(), row[1].as<int64_t>(),
                             row[2].as<std::string>());
    }
    txn.commit();
    return results;
}

// Get a user's score and rank
std::tuple<int64_t, int, std::string> Database::leaderboard_get_user_score(
    const std::string& leaderboard_id, const std::string& game_id, const std::string& user_id) {
    ensure_connection();
    pqxx::work txn(*connection);
    pqxx::result r =
        txn.exec(pqxx::zview("SELECT score, timestamp FROM leaderboards WHERE leaderboard_id = "
                             "$1 AND game_id = $2 AND user_id = "
                             "$3;"),
                 pqxx::params(leaderboard_id, game_id, user_id));
    int64_t score = r.empty() ? 0 : r[0][0].as<int64_t>();
    std::string timestamp = r.empty() ? "" : r[0][1].as<std::string>();
    int rank = -1;
    if (timestamp != "") {
        pqxx::result rank_r =
            txn.exec(pqxx::zview("SELECT COUNT(*) FROM leaderboards WHERE "
                                 "leaderboard_id = $1 AND game_id = $2 AND score > "
                                 "$3;"),
                     pqxx::params(leaderboard_id, game_id, score));
        rank = rank_r[0][0].as<int>() + 1;
    }
    txn.commit();
    return {score, rank, timestamp};
}
std::string Database::get_peer_or_insert(const std::string& reconnection_token,
                                         const std::string& game_id, const std::string& peer_id) {
    ensure_connection();
    pqxx::work txn(*connection);

    // Try to get peer_id from reconnection_token
    pqxx::result r =
        txn.exec(pqxx::zview("SELECT peer_id FROM peers WHERE id = $1 AND game_id = $2;"),
                 pqxx::params(reconnection_token, game_id));
    if (!r.empty()) {
        // Found, return existing peer_id
        std::string found_peer_id = r[0][0].as<std::string>();
        txn.commit();
        return found_peer_id;
    }

    // Not found, insert new peer
    txn.exec(pqxx::zview("INSERT INTO peers (id, peer_id, game_id) VALUES ($1, $2, $3);"),
             pqxx::params(reconnection_token, peer_id, game_id));
    txn.commit();
    return peer_id;
}
