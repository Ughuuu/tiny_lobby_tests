#include "game_thread.h"

#include <ctime>
#include <iomanip>
#include <regex>
#include <sstream>

#include "INIReader.h"
#include "any_type.h"
#include "game_thread_messages.h"
#include "lua.h"

GameThread::GameThread(
    bool verbose, std::string log_folder, std::string scripts_folder,
    moodycamel::BlockingReaderWriterQueue<AnalyticsEvent> &analytics_queue,
    moodycamel::BlockingReaderWriterQueue<WebSocketReceivedMessage> &receive_queue, uWS::Loop *loop,
    WebSocketServer<true> *webserver, WebSocketServer<false> *webserver_no_ssl,
    std::atomic<bool> &stop, int listing_interval, int max_recconection_time)
    : listing_interval(listing_interval),
      max_reconnection_time(max_recconection_time),
      logger(verbose, log_folder + "/game.txt"),
      scripts_folder(scripts_folder),
      analytics_queue(analytics_queue),
      receive_queue(receive_queue),
      loop(loop),
      webserver_ssl(webserver),
      webserver_nossl(webserver_no_ssl),
      stop(stop),
      logs_folder(log_folder),
      games_listener(scripts_folder, file_watcher_queue) {
    file_watcher.addWatch(scripts_folder, &games_listener, true);
    file_watcher.watch();
    logger.debug_log("[GameThread] on_start");
}
std::vector<std::string> extract_bracketed_headers(const std::string &filename) {
    std::vector<std::string> results;
    std::ifstream file(filename);
    std::string line;
    std::regex bracket_regex(R"(\[([^\]]+)\])");

    while (std::getline(file, line)) {
        std::smatch match;
        if (std::regex_search(line, match, bracket_regex)) {
            results.push_back(match[1].str());
        }
    }

    return results;
}

void GameThread::load_games() {
    logger.debug_log("[GameThread] on_load_games");
    if (!std::filesystem::exists("games.ini")) {
        logger.error_log("[GameThread] Cannot open games.ini file");
        return;
    }
    INIReader config_reader("games.ini");
    auto sections = config_reader.Sections();
    for (const auto &section : sections) {
        std::string folder_name = config_reader.GetString(section, "folder", "");
        if (folder_name.empty()) {
            logger.debug_log("[GameThread] on_load_game relay");
        } else {
            logger.debug_log("[GameThread] on_load_game ", folder_name);
        }
        int tickrate = config_reader.GetInteger(section, "tickrate", 0);
        if (tickrate < 16) {
            tickrate = 0;
        }
        std::cout << "Loading game from " << folder_name << " with id " << section << std::endl;
        int sendrate = config_reader.GetInteger(section, "sendrate", 50);
        std::string lobby_control = config_reader.Get(section, "lobby_control", "lua");
        ScriptAS as;
        as.scripts_folder = scripts_folder;
        as.folder_name = folder_name;
        as.script_entrypoint = "main.as";
        as.logs_folder = logs_folder;
        as.game_thread = this;
        as.enabled = lobby_control == "angelscript";
        if (tickrate > 0) {
            check_time = std::min(check_time, tickrate);
        }
        if (sendrate > 0) {
            check_time = std::min(check_time, sendrate);
        }
        games.emplace(section, GameData{.id = section,
                                        .lobby_control = lobby_control,
                                        .tick_rate = tickrate,
                                        .send_rate = sendrate,
                                        .folder_name = folder_name,
                                        .lua = ScriptLua{.scripts_folder = scripts_folder,
                                                         .folder_name = folder_name,
                                                         .script_entrypoint = "main.lua",
                                                         .logs_folder = logs_folder,
                                                         .game_thread = this,
                                                         .enabled = lobby_control == "lua"},
                                        .angelscript = as});
    }
    for (auto &game : games) {
        game.second.open(now);

        if (game.second.enabled_callbacks.find("_on_init") != game.second.enabled_callbacks.end()) {
            bool has_error = false;
            boost::container::vector<AnyElement> args;
            auto func_result = scripted_function_call(EMPTY_STRING, EMPTY_STRING, game.second,
                                                      "_on_init", true, args, has_error);
            if (has_error && std::holds_alternative<std::string>(func_result.value)) {
                logger.error_log("[GameThread] on_error _on_init ",
                                 std::get<std::string>(func_result.value));
            }
        }
    }
}

void GameThread::unload_games() {
    for (auto &game : games) {
        game.second.close();
    }
    logger.debug_log("[GameThread] on_unload_games");
    games.clear();
}

std::string get_current_date() {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y_%m_%d");
    return oss.str();
}

void GameThread::run() {
    now = get_time_now();
    load_games();
    logger.debug_log("[GameThread] on_start_thread");
    int64_t last_listing = now;
    int64_t last_disconnect = now;
    int64_t last_timers = now;
    int64_t last_stats = now;
    int64_t disconnect_interval = max_reconnection_time / 10;
    int64_t timer_interval = 500;
    int64_t stats_interval = 10000;
    int min_process_size = 100;
    int last_time = 0;
    while (!stop) {
        // Process min_process_size messages
        int messages_processed = 0;
        while (receive_queue.size_approx() > 0 && messages_processed < min_process_size) {
            messages_processed++;
            if (!handle_events()) {
                break;
            }
        }
        if (last_time == now) {
            std::this_thread::sleep_for(std::chrono::milliseconds(check_time / 2));
            continue;
        }
        last_time = now;
        if (last_stats + stats_interval < now && logger.verbose) {
            last_stats = now;
            // open stats file and append to it. use date format YYYY-MM-DD for filename
            time_t timestamp = time(&timestamp);

            // check if file is empty and write header if it is
            std::ifstream stats_file_check(logs_folder + "/stats_" + get_current_date() + ".csv");
            if (stats_file_check.peek() == std::ifstream::traits_type::eof()) {
                stats_file_check.close();
                std::ofstream stats_file(logs_folder + "/stats_" + get_current_date() + ".csv");
                stats_file
                    << "timestamp,messages_received,messages_sent,total_users_count,peers_in_game,"
                       "disconnected_peers,anon_users,discord_users,steam_users,total_lobbies_"
                       "count\n";
                stats_file.close();
            } else {
                stats_file_check.close();
            }
            std::ofstream stats_file(logs_folder + "/stats_" + get_current_date() + ".csv",
                                     std::ios::app);
            int total_users_count = 0;
            int anon_users = 0;
            int steam_users = 0;
            int discord_users = 0;
            for (auto &game : games) {
                total_users_count += game.second.peers.size();
                for (auto &peer : game.second.peers) {
                    if (peer.second.platform == "anon") {
                        anon_users++;
                    } else if (peer.second.platform == "steam") {
                        steam_users++;
                    } else if (peer.second.platform == "discord") {
                        discord_users++;
                    }
                }
            }
            int total_lobbies_count = 0;
            int peers_in_game = 0;
            for (auto &game : games) {
                total_lobbies_count += game.second.lobbies.size();
                for (auto &lobby : game.second.lobbies) {
                    peers_in_game += lobby.second.peer_ids.size();
                }
            }
            int disconnected_peers = 0;
            for (auto &game : games) {
                disconnected_peers += game.second.disconnected_peers.size();
            }
            stats_file << now << "," << messages_received << "," << messages_sent << ","
                       << total_users_count << "," << peers_in_game << "," << disconnected_peers
                       << "," << anon_users << "," << discord_users << "," << steam_users << ","
                       << total_lobbies_count << "\n";
            stats_file.close();
            messages_received = 0;
            messages_sent = 0;
        }
        handle_tick();
        handle_send();
        if (now - last_disconnect > disconnect_interval) {
            last_disconnect = now;
            handle_disconnects();
            handle_afk();
        }
        if (now - last_listing > listing_interval) {
            last_listing = now;
            handle_lobby_list();
        }
        if (now - last_timers > timer_interval) {
            last_timers = now;
            handle_timers();
        }
        std::string folder_reloaded;
        if (file_watcher_queue.try_dequeue(folder_reloaded)) {
            reload_game(folder_reloaded);
        }
        // handle tick if/when needed
    }
    unload_games();
}

void GameThread::time_run() {
    using clock = std::chrono::system_clock;
    using ms = std::chrono::milliseconds;

    while (!stop) {
        auto now_local = clock::now();
        auto now_ms = std::chrono::time_point_cast<ms>(now_local);
        auto since_epoch = now_ms.time_since_epoch();
        int64_t next_tick_ms =
            since_epoch.count() + (check_time - (since_epoch.count() % check_time));
        auto wake_time = clock::time_point(ms(next_tick_ms));

        std::this_thread::sleep_until(wake_time);
        now = next_tick_ms;
    }
}

std::string join(const boost::container::vector<std::string> &vec, const std::string &delimiter) {
    if (vec.empty()) return "";

    std::ostringstream oss;
    auto it = vec.begin();
    oss << *it;  // First element
    ++it;

    for (; it != vec.end(); ++it) {
        oss << delimiter << *it;
    }
    return oss.str();
}

void GameThread::handle_send() {
    for (auto &game : games) {
        auto &game_data = game.second;
        if (game_data.send_rate == 0 || game_data.last_send_time + game_data.send_rate > now) {
            continue;
        }
        game_data.last_send_time += game_data.send_rate;

        if (webserver_ssl != nullptr) {
            loop->defer([peers_send_data = std::move(game_data.peers_send_data),
                         webserver = webserver_ssl]() {
                for (auto &peer : peers_send_data) {
                    if (peer.second.empty()) {
                        continue;
                    }
                    std::string batched_message = "[" + join(peer.second, ",") + "]";
                    webserver->send(peer.first, batched_message, uWS::OpCode::TEXT);
                }
            });
        } else {
            loop->defer([peers_send_data = std::move(game_data.peers_send_data),
                         webserver = webserver_nossl]() {
                for (auto &peer : peers_send_data) {
                    if (peer.second.empty()) {
                        continue;
                    }
                    std::string batched_message = "[" + join(peer.second, ",") + "]";
                    webserver->send(peer.first, batched_message, uWS::OpCode::TEXT);
                }
            });
        }
        game_data.peers_send_data.clear();
    }
}

void GameThread::handle_tick() {
    int64_t previous_tick = now;
    for (auto &game : games) {
        auto &game_data = game.second;
        if (game_data.tick_rate == 0 ||
            game_data.last_tick_time + game_data.tick_rate > previous_tick) {
            continue;
        }
        boost::container::vector<AnyElement> args(1);
        args[0] = AnyElement{game_data.tick_rate};
        game_data.last_tick_time += game_data.tick_rate;
        now = game_data.last_tick_time;

        for (auto &lobby : game_data.lobbies) {
            auto &lobby_id = lobby.first;
            bool has_error = false;
            auto result = scripted_function_call(EMPTY_STRING, lobby_id, game_data, "_on_tick",
                                                 true, args, has_error);
            if (has_error || std::holds_alternative<std::string>(result.value)) {
                on_error(game_data, EMPTY_STRING, EMPTY_STRING, std::get<std::string>(result.value),
                         false, true);
            }
        }
    }
    now = previous_tick;
}

void GameThread::handle_timers() {
    for (auto &game : games) {
        for (auto &timer_data : game.second.timer_data) {
            if (timer_data.second.end_time < now) {
                bool has_error = false;
                auto timer_data_obj = timer_data.second;
                // lobby got destroyed
                if (game.second.lobbies.find(timer_data.second.lobby_id) ==
                    game.second.lobbies.end()) {
                    on_error(game.second, EMPTY_STRING, timer_data.second.peer_id,
                             ERROR_LOBBY_NOT_FOUND, true);
                    game.second.timer_data.erase(timer_data.first);
                    continue;
                }
                // peer got destroyed
                if (game.second.peers.find(timer_data.second.peer_id) == game.second.peers.end()) {
                    on_error(game.second, EMPTY_STRING, timer_data.second.peer_id,
                             ERROR_PEER_NOT_FOUND, true);
                    game.second.timer_data.erase(timer_data.first);
                    continue;
                }
                game.second.timer_data.erase(timer_data.first);
                auto result = scripted_function_call(
                    timer_data_obj.peer_id, timer_data_obj.lobby_id, game.second, timer_data_obj.id,
                    true, timer_data_obj.args, has_error);
                if (has_error || std::holds_alternative<std::string>(result.value)) {
                    on_error(game.second, EMPTY_STRING, timer_data_obj.peer_id,
                             std::get<std::string>(result.value), true);
                }
            }
        }
    }
}

bool GameThread::handle_events() {
    WebSocketReceivedMessage message;
    if (!receive_queue.wait_dequeue_timed(message, check_time * 500)) {
        return false;
    }
    messages_received++;
    if (games.find(message.game_id) == games.end()) {
        auto fake_game = GameData{.send_rate = 0};
        on_error(fake_game, EMPTY_STRING, message.id, ERROR_GAME_NOT_FOUND + message.game_id, true);
        return true;
    }
    auto &game = games[message.game_id];
    switch (message.event) {
        case WebSocketEvent::OPEN:
            on_connect(game, message.id, message.game_id, message.reconnection_token,
                       message.platform, message.platform_id, message.name);
            break;
        case WebSocketEvent::CLOSE:
            on_close(game, message.id);
            break;
        case WebSocketEvent::MESSAGE:
            // decode message
            yyjson_doc *doc = yyjson_read(message.message.c_str(), message.message.size(), 0);
            if (!doc) {
                on_error(game, EMPTY_STRING, message.id,
                         ERROR_CANNOT_PARSE_JSON + " Initial decode", true);
                break;
            }

            yyjson_val *root = yyjson_doc_get_root(doc);
            if (!root || !yyjson_is_obj(root)) {
                yyjson_doc_free(doc);
                on_error(game, EMPTY_STRING, message.id,
                         ERROR_CANNOT_PARSE_JSON + " Root is not object", true);
                break;
            }
            // get command
            yyjson_val *command_val = yyjson_obj_get(root, "command");
            if (!command_val || !yyjson_is_str(command_val)) {
                yyjson_doc_free(doc);
                on_error(game, EMPTY_STRING, message.id,
                         ERROR_CANNOT_PARSE_JSON + " Command missing or not string", true);
                break;
            }

            std::string command = yyjson_get_str(command_val);
            yyjson_val *data_val = yyjson_obj_get(root, "data");
            std::string command_id;
            if (data_val && yyjson_is_obj(data_val)) {
                yyjson_val *id_val = yyjson_obj_get(data_val, "id");
                if (id_val && yyjson_is_str(id_val)) {
                    command_id = yyjson_get_str(id_val);
                }
            }
            if (game.peers.find(message.id) == game.peers.end()) {
                yyjson_doc_free(doc);
                on_error(game, command_id, message.id, ERROR_PEER_NOT_FOUND + " when reading input",
                         true);
                break;
            }
            auto &peer = game.peers[message.id];
            // set last message time
            peer.last_message_time = now;
            // relay
            if (command == "lobby_data") {
                on_lobby_data(game, std::string(command_id), peer, data_val);
            } else if (command == "data_to") {
                on_data_to(game, std::string(command_id), peer, data_val);
            } else if (command == "data_to_all") {
                on_data_to_all(game, std::string(command_id), peer, data_val);
            } else if (command == "notify_to") {
                on_notify_to(game, std::string(command_id), peer, data_val);
            } else if (command == "lobby_notify") {
                on_lobby_notify(game, std::string(command_id), peer, data_val);
            }
            // scripted
            else if (command == "lobby_call") {
                on_lobby_call(game, std::string(command_id), peer, data_val);
            } else if (command == "quick_join") {
                on_quick_join(game, std::string(command_id), peer, data_val);
            } else if (command == "create_lobby") {
                on_create_lobby(game, std::string(command_id), peer, data_val);
            } else if (command == "join_lobby") {
                on_join_lobby(game, std::string(command_id), peer, data_val);
            } else if (command == "leave_lobby") {
                on_leave_lobby(game, std::string(command_id), peer, data_val);
            } else if (command == "list_lobby") {
                on_list_lobby(game, std::string(command_id), peer, data_val);
            } else if (command == "chat_lobby") {
                on_chat_lobby(game, std::string(command_id), peer, data_val);
            } else if (command == "lobby_tags") {
                on_lobby_tags(game, std::string(command_id), peer, data_val);
            } else if (command == "kick_peer") {
                on_kick_peer(game, std::string(command_id), peer, data_val);
            } else if (command == "user_data") {
                on_user_data(game, std::string(command_id), peer, data_val);
            } else if (command == "lobby_ready") {
                on_lobby_ready(game, std::string(command_id), peer, data_val);
            } else if (command == "lobby_unready") {
                on_lobby_unready(game, std::string(command_id), peer, data_val);
            } else if (command == "seal_lobby") {
                on_seal_lobby(game, std::string(command_id), peer, data_val);
            } else if (command == "unseal_lobby") {
                on_unseal_lobby(game, std::string(command_id), peer, data_val);
            } else if (command == "lobby_max_players") {
                on_lobby_max_players(game, std::string(command_id), peer, data_val);
            } else if (command == "lobby_title") {
                on_lobby_title(game, std::string(command_id), peer, data_val);
            } else if (command == "lobby_password") {
                on_lobby_password(game, std::string(command_id), peer, data_val);
            } else {
                on_error(game, std::string(command_id), message.id,
                         ERROR_UNKOWN_COMMAND + " " + std::string(command), true);
            }
            yyjson_doc_free(doc);
            break;
    }
    return true;
}

void GameThread::handle_disconnects() {
    for (auto &game : games) {
        auto &game_data = game.second;
        boost::container::flat_set<std::string> to_erase;
        for (auto &peer : game_data.disconnected_peers) {
            auto &peer_obj = game_data.peers[peer.first];
            // peer no longer in lobby
            if ((peer_obj.lobby_id == EMPTY_STRING ||
                 game_data.lobbies.find(peer_obj.lobby_id) == game_data.lobbies.end()) &&
                (now - peer.second > max_reconnection_time)) {
                peer_obj.leave_lobby();
                to_erase.insert(peer.first);
                continue;
            }
            auto &lobby = game_data.lobbies[peer_obj.lobby_id];
            if (now - peer.second > max_reconnection_time) {
                remove_peer_from_lobby(game_data, lobby, peer_obj.id, EMPTY_STRING, true);
                to_erase.insert(peer.first);
            }
        }
        // send disconnected users to websocket server
        if (webserver_ssl != nullptr) {
            loop->defer([to_erase = to_erase, webserver = webserver_ssl]() {
                webserver->clear_users(to_erase);
            });
        } else {
            loop->defer([to_erase = to_erase, webserver = webserver_nossl]() {
                webserver->clear_users(to_erase);
            });
        }
        // clear them from game data
        for (auto &peer_id : to_erase) {
            game_data.disconnected_peers.erase(peer_id);
        }
        for (auto &peer_id : to_erase) {
            game_data.peers.erase(peer_id);
        }
    }
}

void GameThread::handle_afk() {
    for (auto &game : games) {
        auto &game_data = game.second;
        boost::container::flat_set<std::string> to_erase;
        for (auto &peer : game_data.peers) {
            auto &peer_obj = peer.second;
            if (game_data.lobbies.find(peer_obj.lobby_id) == game_data.lobbies.end()) {
                continue;
            }
            auto &lobby = game_data.lobbies[peer_obj.lobby_id];
            bool destroy_lobby = true;
            for (auto &peer_id : lobby.peer_ids) {
                auto &lobby_peer = game_data.peers[peer_id];
                // if peer is not afk, do not destroy lobby
                if (now - peer_obj.last_message_time < max_reconnection_time) {
                    destroy_lobby = false;
                    break;
                }
            }
            if (destroy_lobby) {
                remove_peer_from_lobby(game_data, game_data.lobbies[lobby.id], lobby.host,
                                       EMPTY_STRING, false);
            }
        }
    }
}

void GameThread::handle_lobby_list() {
    for (auto &game : games) {
        auto &game_data = game.second;
        if (game_data.lobbies_updated.empty() || game_data.lobby_listing_peers.empty()) {
            game_data.lobbies_updated.clear();
            continue;
        }
        boost::container::vector<AnyElement> lobbies;
        for (const auto &lobby_id : game_data.lobbies_updated) {
            // if deleted, only send id
            if (game_data.lobbies.find(lobby_id) == game_data.lobbies.end()) {
                boost::container::flat_map<std::string, AnyElement> lobby_data;
                lobby_data.insert({"id", AnyElement{lobby_id}});
                lobbies.push_back(AnyElement{lobby_data});
                continue;
            }
            auto &lobby = game_data.lobbies[lobby_id];
            lobbies.push_back(AnyElement{lobby.to_dict()});
        }

        std::string notification = notification_lobby_list(AnyElement{lobbies}, EMPTY_STRING);
        for (auto &peer : game_data.lobby_listing_peers) {
            send(game_data, peer, notification, uWS::OpCode::TEXT);
        }
        game_data.lobbies_updated.clear();
    }
}

void GameThread::remove_peer_from_lobby(GameData &game, LobbyData &lobby, std::string &peer_id,
                                        const std::string &command_id, bool kicked) {
    bool is_host = peer_id == lobby.host;
    if (is_host) {
        std::string notification_others = notification_lobby_kicked();
        for (auto lobby_peer_id : lobby.peer_ids) {
            auto &lobby_peer = game.peers[lobby_peer_id];
            lobby_peer.leave_lobby();
            if (lobby_peer_id == peer_id) {
                continue;
            }
            send(game, lobby_peer_id, notification_others, uWS::OpCode::TEXT);
        }
        std::string notification_self = notification_lobby_left(command_id);
        send(game, peer_id, notification_self, uWS::OpCode::TEXT);
    } else {
        std::string notification_others = notification_peer_left(peer_id, EMPTY_STRING);
        if (kicked) {
            notification_others = notification_peer_kicked(peer_id, EMPTY_STRING);
        }
        for (auto lobby_peer_id : lobby.peer_ids) {
            auto &lobby_peer = game.peers[lobby_peer_id];
            if (lobby_peer_id == peer_id) {
                lobby_peer.leave_lobby();
            }
            // if you got kicked, the host gets command notified also
            if (kicked && lobby_peer_id == lobby.host) {
                continue;
            }
            if (lobby_peer_id == peer_id) {
                continue;
            }
            send(game, lobby_peer_id, notification_others, uWS::OpCode::TEXT);
        }
        std::string notification_left = notification_lobby_left(command_id);
        if (kicked) {
            notification_left = notification_lobby_kicked();
        }
        send(game, peer_id, notification_left, uWS::OpCode::TEXT);
        // if kicked, notify host of peer_kicked with command id
        if (kicked) {
            std::string notification_host = notification_peer_kicked(peer_id, command_id);
            send(game, lobby.host, notification_host, uWS::OpCode::TEXT);
        }
    }
    lobby.peer_ids.erase(peer_id);
    if (game.enabled_callbacks.find("_on_left") != game.enabled_callbacks.end()) {
        bool has_error = false;
        boost::container::vector<AnyElement> args;
        auto func_result =
            scripted_function_call(peer_id, lobby.id, game, "_on_left", true, args, has_error);
        if (has_error && std::holds_alternative<std::string>(func_result.value)) {
            on_error(game, command_id, peer_id, std::get<std::string>(func_result.value), false,
                     has_error);
        }
    }
    if (is_host) {
        game.lobbies_updated.insert(lobby.id);
        game.lobbies.erase(lobby.id);
    }
}

void GameThread::send(GameData &game, const std::string &peer_id, const std::string &message,
                      uWS::OpCode opCode) {
    // check if peer is disconnected
    if (game.disconnected_peers.find(peer_id) != game.disconnected_peers.end() &&
        opCode != uWS::OpCode::CLOSE) {
        return;
    }
    // send immediately or batch
    if (game.send_rate == 0 || opCode == uWS::OpCode::CLOSE) {
        if (webserver_ssl != nullptr) {
            loop->defer([id = peer_id, msg = message, webserver = webserver_ssl, opCode]() {
                webserver->send(id, msg, opCode);
            });
        } else {
            loop->defer([id = peer_id, msg = message, webserver = webserver_nossl, opCode]() {
                webserver->send(id, msg, opCode);
            });
        }
    } else {
        auto &msg_list = game.peers_send_data[peer_id];

        if (msg_list.capacity() == 0) {
            msg_list.reserve(32);
        }

        msg_list.emplace_back(message);
    }
    messages_sent++;
}

void GameThread::on_connect(GameData &game, std::string &peer_id, std::string &game_id,
                            std::string &reconnection_token, std::string &platform,
                            std::string &platform_id, std::string &name) {
    logger.debug_log("[GameThread] on_connect ", peer_id, " ", game_id);
    if (game.peers.find(peer_id) != game.peers.end()) {
        auto &peer = game.peers[peer_id];
        peer.reconnection_token = reconnection_token;
        game.disconnected_peers.erase(peer_id);
    } else {
        game.peers.emplace(peer_id, PeerData{
                                        .id = peer_id,
                                        .game_id = game_id,
                                        .reconnection_token = reconnection_token,
                                        .platform = platform,
                                        .platform_id = platform_id,
                                    });
    }
    auto &peer = game.peers[peer_id];
    peer.user_data["name"] = AnyElement{name};
    std::string notification = notification_peer_state(AnyElement{peer.to_dict(true, true)});
    send(game, peer.id, notification, uWS::OpCode::TEXT);
}

void GameThread::on_close(GameData &game, std::string &peer_id) {
    logger.debug_log("[GameThread] on_close ", peer_id);
    if (game.peers.find(peer_id) == game.peers.end()) {
        return;
    }
    auto &peer = game.peers[peer_id];
    peer.ready = false;
    peer.disconnected = true;
    game.disconnected_peers.emplace(peer_id, now);
    std::string notification = notification_peer_disconnected(peer_id);
    if (game.lobbies.find(peer.lobby_id) != game.lobbies.end()) {
        auto &lobby = game.lobbies[peer.lobby_id];
        for (auto lobby_peer_id : lobby.peer_ids) {
            if (lobby_peer_id == peer_id) {
                continue;
            }
            send(game, lobby_peer_id, notification, uWS::OpCode::TEXT);
        }
    }
}

void GameThread::on_error(GameData &game, std::string command_id, std::string peer_id,
                          std::string message, bool close, bool logical_error) {
    if (close) {
        logger.error_log("[GameThread] on_error ", command_id, " ", peer_id, " ", message);
    }
    if (close) {
        send(game, peer_id, message, uWS::OpCode::CLOSE);
    } else {
        send(game, peer_id, notification_error(message, command_id, logical_error),
             uWS::OpCode::TEXT);
    }
}

void GameThread::on_lobby_call(GameData &game, std::string command_id, PeerData &peer,
                               yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_lobby_call ", command_id, " ", peer.id);
    if (peer.lobby_id == "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto func_name = decode_string_or_default(data_val, "function", "");
    if (func_name == "") {
        return on_error(game, command_id, peer.id, ERROR_INVALID_FUNCTION_MSG);
    }
    AnyElement args{boost::container::vector<AnyElement>()};
    yyjson_val *inputs_val = yyjson_obj_get(data_val, "inputs");
    if (inputs_val && yyjson_is_arr(inputs_val)) {
        decode_array(inputs_val, args);
    }
    if (!std::holds_alternative<boost::container::vector<AnyElement>>(args.value)) {
        return on_error(game, command_id, peer.id, ERROR_INVALID_ARGUMENTS);
    }
    auto &array_value = std::get<boost::container::vector<AnyElement>>(args.value);
    // SCRIPTED CALL
    bool has_error = false;
    auto func_result = scripted_function_call(peer.id, peer.lobby_id, game, func_name, true,
                                              array_value, has_error);
    if (has_error && std::holds_alternative<std::string>(func_result.value)) {
        return on_error(game, command_id, peer.id, std::get<std::string>(func_result.value), false,
                        has_error);
    } else {
        std::string notification = notification_lobby_call(func_result, command_id);
        send(game, peer.id, notification, uWS::OpCode::TEXT);
    }
}

bool lobby_contains_tags(const boost::container::flat_map<std::string, AnyElement> &joining_tags,
                         const boost::container::flat_map<std::string, AnyElement> &lobby_tags) {
    for (auto &tag : joining_tags) {
        if (lobby_tags.find(tag.first) == lobby_tags.end()) {
            return false;
        }
        auto &lobby_tag_value = lobby_tags.at(tag.first).value;
        auto &joining_tag_value = tag.second.value;
        if (lobby_tag_value != joining_tag_value) {
            return false;
        }
    }
    return true;
}

void GameThread::on_quick_join(GameData &game, std::string command_id, PeerData &peer,
                               yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_quick_join ", command_id, " ", peer.id);
    if (peer.lobby_id != "") {
        if (on_join_lobby(game, command_id, peer, data_val)) {
            return;
        }
    }
    boost::container::flat_map<std::string, AnyElement> lobby_tags;
    std::string decode_error = decode_object(yyjson_obj_get(data_val, "tags"), lobby_tags);
    if (decode_error != "") {
        return on_error(game, command_id, peer.id, "Invalid tags " + decode_error);
    }
    // TODO optimization keep lobbies open
    for (auto &lobby_obj : game.lobbies) {
        auto &lobby = lobby_obj.second;
        if (lobby.max_players > lobby.peer_ids.size() && !lobby.sealed && !lobby.password.size() &&
            (lobby.host == "" || !game.peers[lobby.host].disconnected) &&
            lobby_contains_tags(lobby_tags, lobby.tags)) {
            if (on_join_lobby(game, command_id, peer, data_val, lobby.id)) {
                return;
            }
        }
    }
    on_create_lobby(game, command_id, peer, data_val, &lobby_tags);
}

void GameThread::on_create_lobby(
    GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val,
    boost::container::flat_map<std::string, AnyElement> *previous_tags) {
    logger.debug_log("[GameThread] on_create_lobby ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id != "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_IS_IN_A_LOBBY + " " + peer.lobby_id);
    }
    // CHANGES before scripted call, reverted if scripted call fails
    auto uuid = to_string(gen());
    auto small_uuid = uuid.substr(0, 8);
    boost::container::flat_map<std::string, AnyElement> lobby_tags;
    if (previous_tags != nullptr) {
        lobby_tags = *previous_tags;
    } else {
        std::string decode_error = decode_object(yyjson_obj_get(data_val, "tags"), lobby_tags);
        if (decode_error != "") {
            return on_error(game, command_id, peer.id, "Invalid tags " + decode_error);
        }
    }
    peer.lobby_id = small_uuid;
    game.lobbies.emplace(small_uuid,
                         LobbyData{
                             .id = small_uuid,
                             .name = decode_string_or_default(data_val, "name", ""),
                             .host = peer.id,
                             .password = decode_string_or_default(data_val, "password", ""),
                             .max_players = decode_int_or_default(data_val, "max_players", 0),
                             .peer_ids = {peer.id},
                             .create_time = now,
                             .game_id = peer.game_id,
                             .tags = lobby_tags,
                         });
    game.lobby_listing_peers.erase(peer.id);
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_can_create") != game.enabled_callbacks.end()) {
        bool has_error = false;
        boost::container::vector<AnyElement> args;
        auto func_result = scripted_function_call(peer.id, peer.lobby_id, game, "_can_create", true,
                                                  args, has_error);
        if (has_error && std::holds_alternative<std::string>(func_result.value)) {
            // revert the changes
            peer.leave_lobby();
            game.lobbies.erase(small_uuid);
            return on_error(game, command_id, peer.id, std::get<std::string>(func_result.value),
                            false, has_error);
        }
    }
    game.lobbies_updated.insert(peer.lobby_id);
    peer.disconnected = false;
    // NOTIFICATION
    std::string notification =
        notification_lobby_created(AnyElement{game.lobbies[small_uuid].to_dict()},
                                   AnyElement{game.peers_to_array(small_uuid)}, command_id);
    send(game, peer.id, notification, uWS::OpCode::TEXT);
    // STATISTICS
    analytics_queue.enqueue(AnalyticsEvent{
        .event = "lobby_created",
        .event_data =
            boost::container::flat_map<std::string, AnyElement>{
                {"max_players", AnyElement{game.lobbies[small_uuid].max_players}},
                {"game_id", AnyElement{game.lobbies[small_uuid].game_id}},
                {"has_password", AnyElement{!game.lobbies[small_uuid].password.empty()}}},
        .event_flag = "created",
        .event_key = "lobby_created",
        .event_type = "lobby",
        .sub_event = "",
    });
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_on_create") != game.enabled_callbacks.end()) {
        bool has_error = false;
        boost::container::vector<AnyElement> args;
        auto func_result = scripted_function_call(peer.id, peer.lobby_id, game, "_on_create", true,
                                                  args, has_error);
        if (has_error && std::holds_alternative<std::string>(func_result.value)) {
            on_error(game, command_id, peer.id, std::get<std::string>(func_result.value), false,
                     has_error);
        }
    }
}

bool GameThread::on_join_lobby(GameData &game, std::string command_id, PeerData &peer,
                               yyjson_val *data_val, std::string lobby_id_override) {
    logger.debug_log("[GameThread] on_join_lobby ", command_id, " ", peer.id);
    // PRECONDITIONS
    bool reconnecting = false;
    std::string lobby_id = lobby_id_override;
    if (lobby_id_override.empty()) {
        lobby_id = decode_string_or_default(data_val, "lobby_id", "");
    }
    if (peer.disconnected && peer.lobby_id != "") {
        reconnecting = true;
        lobby_id = peer.lobby_id;
    }
    if (lobby_id == "" || game.lobbies.find(lobby_id) == game.lobbies.end()) {
        on_error(game, command_id, peer.id, "Invalid lobby id");
        return false;
    }
    auto &lobby = game.lobbies[lobby_id];
    if (!reconnecting) {
        if (peer.lobby_id != "") {
            on_error(game, command_id, peer.id, ERROR_PEER_IS_IN_A_LOBBY);
            return false;
        }
        if (lobby.password != decode_string_or_default(data_val, "password", "")) {
            on_error(game, command_id, peer.id, "Invalid password");
            return false;
        }
        if (lobby.max_players != 0 && lobby.peer_ids.size() >= lobby.max_players) {
            on_error(game, command_id, peer.id, "Lobby is full");
            return false;
        }
    }
    if (game.enabled_callbacks.find("_can_join") != game.enabled_callbacks.end()) {
        bool has_error = false;
        boost::container::vector<AnyElement> args;
        auto func_result =
            scripted_function_call(peer.id, lobby_id, game, "_can_join", true, args, has_error);
        if (has_error && std::holds_alternative<std::string>(func_result.value)) {
            on_error(game, command_id, peer.id, std::get<std::string>(func_result.value), false,
                     has_error);
            return false;
        }
    }
    // CHANGES
    game.lobby_listing_peers.erase(peer.id);
    if (reconnecting) {
        peer.ready = false;
    } else {
        peer.order_id = ++lobby.order_id_counter;
        peer.lobby_id = lobby_id;
        lobby.peer_ids.insert(peer.id);
    }
    peer.disconnected = false;
    // NOTIFICATION
    if (reconnecting) {
        std::string notification = notification_peer_reconnected(peer.id);
        for (auto lobby_peer_id : lobby.peer_ids) {
            if (lobby_peer_id == peer.id) {
                continue;
            }
            send(game, lobby_peer_id, notification, uWS::OpCode::TEXT);
        }
    } else {
        std::string notification_others = notification_peer_joined(AnyElement{peer.to_dict()});
        for (auto lobby_peer_id : lobby.peer_ids) {
            if (lobby_peer_id == peer.id) {
                continue;
            }
            send(game, lobby_peer_id, notification_others, uWS::OpCode::TEXT);
        }
    }
    // if it's relay, send private lobby data too at connection
    std::string notification_self =
        notification_lobby_joined(AnyElement{lobby.to_dict(game.lobby_control == "relay")},
                                  AnyElement{game.peers_to_array(lobby.id)}, command_id);
    send(game, peer.id, notification_self, uWS::OpCode::TEXT);
    // SCRIPTED CALL
    if (!reconnecting) {
        if (game.enabled_callbacks.find("_on_join") != game.enabled_callbacks.end()) {
            bool has_error = false;
            boost::container::vector<AnyElement> args;
            auto func_result =
                scripted_function_call(peer.id, lobby_id, game, "_on_join", true, args, has_error);
            if (has_error && std::holds_alternative<std::string>(func_result.value)) {
                on_error(game, command_id, peer.id, std::get<std::string>(func_result.value), false,
                         has_error);
            }
        }
    }
    return true;
}

void GameThread::on_leave_lobby(GameData &game, std::string command_id, PeerData &peer,
                                yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_leave_lobby ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    // CHANGES
    remove_peer_from_lobby(game, lobby, peer.id, command_id);
}

void GameThread::on_list_lobby(GameData &game, std::string command_id, PeerData &peer,
                               yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_list_lobby ", command_id, " ", peer.id);
    if (game.lobby_listing_peers.find(peer.id) != game.lobby_listing_peers.end()) {
        std::string notification =
            notification_lobby_list(AnyElement{boost::container::vector<AnyElement>{}}, command_id);
        return send(game, peer.id, notification, uWS::OpCode::TEXT);
    }

    game.lobby_listing_peers.insert(peer.id);

    boost::container::vector<LobbyData *> lobby_array;
    for (auto &lobby_pair : game.lobbies) {
        auto &lobby = lobby_pair.second;
        if (!lobby.sealed || peer.lobby_id == lobby.id) {
            lobby_array.push_back(&lobby);
        }
    }

    // Sort the array based on create time
    std::sort(lobby_array.begin(), lobby_array.end(),
              [](LobbyData *a, LobbyData *b) { return a->create_time < b->create_time; });

    // Select a maximum number of lobbies to get
    int max_lobbies_to_get = std::min(100, static_cast<int>(lobby_array.size()));
    boost::container::vector<std::string> selected_lobbies;
    for (int i = 0; i < max_lobbies_to_get; ++i) {
        selected_lobbies.push_back(lobby_array[i]->id);
    }

    // Create lobby objects
    boost::container::vector<AnyElement> lobbies;
    for (const auto &lobby_id : selected_lobbies) {
        auto &lobby = game.lobbies[lobby_id];
        lobbies.push_back(AnyElement{lobby.to_dict()});
    }

    std::string notification = notification_lobby_list(AnyElement{lobbies}, command_id);
    send(game, peer.id, notification, uWS::OpCode::TEXT);
}

std::string strip_BBCode(const std::string &input) {
    static const std::regex bbcode_regex(R"(\[.*?\])");
    return std::regex_replace(input, bbcode_regex, "");
}

void GameThread::on_chat_lobby(GameData &game, std::string command_id, PeerData &peer,
                               yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_chat_lobby ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    std::string message = decode_string_or_default(data_val, "chat", "");
    message = strip_BBCode(message);
    if (message == "" || message.size() > 256) {
        return on_error(game, command_id, peer.id, "Invalid chat message");
    }
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_can_chat") != game.enabled_callbacks.end()) {
        bool has_error = false;
        boost::container::vector<AnyElement> args(1);
        args[0] = AnyElement{message};
        auto func_result = scripted_function_call(peer.id, peer.lobby_id, game, "_can_chat", true,
                                                  args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(game, command_id, peer.id, std::get<std::string>(func_result.value),
                            false, has_error);
        }
    }
    // NOTIFICATION
    std::string notification_others = notification_chat(peer.id, message, EMPTY_STRING);
    for (auto lobby_peer_id : game.lobbies[peer.lobby_id].peer_ids) {
        if (lobby_peer_id == peer.id) {
            continue;
        }
        send(game, lobby_peer_id, notification_others, uWS::OpCode::TEXT);
    }
    std::string notification_self = notification_chat(peer.id, message, command_id);
    send(game, peer.id, notification_self, uWS::OpCode::TEXT);
}

void GameThread::on_lobby_tags(GameData &game, std::string command_id, PeerData &peer,
                               yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_lobby_tags ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    if (lobby.host != peer.id) {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_HOST);
    }
    boost::container::flat_map<std::string, AnyElement> new_tags;
    if (!data_val || !yyjson_is_obj(data_val)) {
        return on_error(game, command_id, peer.id,
                        ERROR_CANNOT_PARSE_JSON + " Data is missing or not an object");
    }
    yyjson_val *tags_val = yyjson_obj_get(data_val, "tags");
    if (!tags_val || !yyjson_is_obj(tags_val)) {
        return on_error(game, command_id, peer.id,
                        ERROR_CANNOT_PARSE_JSON + " Lobby tags missing or not object");
    }
    std::string error = decode_object(tags_val, new_tags);
    if (!error.empty()) {
        return on_error(game, command_id, peer.id, error);
    }
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_can_tags") != game.enabled_callbacks.end()) {
        bool has_error = false;
        boost::container::vector<AnyElement> args(1);
        args[0] = AnyElement{new_tags};
        auto func_result = scripted_function_call(peer.id, peer.lobby_id, game, "_can_tags", true,
                                                  args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(game, command_id, peer.id, std::get<std::string>(func_result.value),
                            false, has_error);
        }
    }
    // CHANGES
    // only update new tags
    for (const auto &tag_pair : new_tags) {
        if (std::holds_alternative<std::monostate>(tag_pair.second.value)) {
            lobby.tags.erase(tag_pair.first);
            continue;
        }
        lobby.tags[tag_pair.first] = tag_pair.second;
    }
    // NOTIFICATION
    std::string notification_others = notification_tags(AnyElement{lobby.tags}, EMPTY_STRING);
    for (auto lobby_peer_id : game.lobbies[peer.lobby_id].peer_ids) {
        if (lobby_peer_id == peer.id) {
            continue;
        }
        send(game, lobby_peer_id, notification_others, uWS::OpCode::TEXT);
    }
    std::string notification_self = notification_tags(AnyElement{lobby.tags}, command_id);
    send(game, peer.id, notification_self, uWS::OpCode::TEXT);
}

void GameThread::on_kick_peer(GameData &game, std::string command_id, PeerData &peer,
                              yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_kick_peer ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    if (lobby.host != peer.id) {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_HOST);
    }
    std::string kicked_peer_id = decode_string_or_default(data_val, "peer_id", "");
    if (kicked_peer_id == "" || lobby.peer_ids.find(kicked_peer_id) == lobby.peer_ids.end()) {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    if (peer.id == kicked_peer_id) {
        return on_error(game, command_id, peer.id, ERROR_CANNOT_KICK_SELF);
    }
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_can_kick") != game.enabled_callbacks.end()) {
        bool has_error = false;
        boost::container::vector<AnyElement> args(1);
        args[0] = AnyElement{kicked_peer_id};
        auto func_result = scripted_function_call(peer.id, peer.lobby_id, game, "_can_kick", true,
                                                  args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(game, command_id, peer.id, std::get<std::string>(func_result.value),
                            false, has_error);
        }
    }
    remove_peer_from_lobby(game, lobby, kicked_peer_id, command_id, true);
}

void GameThread::on_user_data(GameData &game, std::string command_id, PeerData &peer,
                              yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_user_data ", command_id, " ", peer.id);
    // CHANGES
    boost::container::flat_map<std::string, AnyElement> new_userdata;
    if (!data_val || !yyjson_is_obj(data_val)) {
        return on_error(game, command_id, peer.id,
                        ERROR_CANNOT_PARSE_JSON + " Data is missing or not an object");
    }
    yyjson_val *userdata_val = yyjson_obj_get(data_val, "user_data");
    if (!userdata_val || !yyjson_is_obj(userdata_val)) {
        return on_error(game, command_id, peer.id,
                        ERROR_CANNOT_PARSE_JSON + " Lobby tags missing or not object");
    }
    std::string error = decode_object(userdata_val, new_userdata);
    if (!error.empty()) {
        return on_error(game, command_id, peer.id, error);
    }
    for (const auto &userdata_pair : new_userdata) {
        if (std::holds_alternative<std::monostate>(userdata_pair.second.value)) {
            peer.user_data.erase(userdata_pair.first);
            continue;
        }
        peer.user_data[userdata_pair.first] = userdata_pair.second;
    }
    // NOTIFICATION
    if (peer.lobby_id != "") {
        auto &lobby = game.lobbies[peer.lobby_id];
        std::string notification_others =
            notification_user_data(AnyElement{peer.user_data}, peer.id, EMPTY_STRING);
        for (auto lobby_peer_id : game.lobbies[peer.lobby_id].peer_ids) {
            if (lobby_peer_id == peer.id) {
                continue;
            }
            send(game, lobby_peer_id, notification_others, uWS::OpCode::TEXT);
        }
    }
    std::string notification_self =
        notification_user_data(AnyElement{peer.user_data}, peer.id, command_id);
    send(game, peer.id, notification_self, uWS::OpCode::TEXT);
}

void GameThread::on_lobby_ready(GameData &game, std::string command_id, PeerData &peer,
                                yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_lobby_ready ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    if (peer.ready) {
        return on_error(game, command_id, peer.id, ERROR_PEER_IS_READY);
    }
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_can_ready") != game.enabled_callbacks.end()) {
        bool has_error = false;
        boost::container::vector<AnyElement> args(1);
        args[0] = AnyElement{true};
        auto func_result = scripted_function_call(peer.id, peer.lobby_id, game, "_can_ready", true,
                                                  args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(game, command_id, peer.id, std::get<std::string>(func_result.value),
                            false, has_error);
        }
    }
    set_lobby_ready(game, game.lobbies[peer.lobby_id], peer, command_id, true);
}

void GameThread::on_lobby_unready(GameData &game, std::string command_id, PeerData &peer,
                                  yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_lobby_unready ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    if (!peer.ready) {
        return on_error(game, command_id, peer.id, ERROR_PEER_IS_NOT_READY);
    }
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_can_ready") != game.enabled_callbacks.end()) {
        bool has_error = false;
        boost::container::vector<AnyElement> args(1);
        args[0] = AnyElement{false};
        auto func_result = scripted_function_call(peer.id, peer.lobby_id, game, "_can_ready", true,
                                                  args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(game, command_id, peer.id, std::get<std::string>(func_result.value),
                            false, has_error);
        }
    }
    set_lobby_ready(game, game.lobbies[peer.lobby_id], peer, command_id, false);
}

void GameThread::set_lobby_ready(GameData &game, LobbyData &lobby, PeerData &peer,
                                 std::string command_id, bool ready) {
    // CHANGES
    peer.ready = ready;
    // NOTIFICATION
    std::string notification_others = notification_peer_ready(peer.id, EMPTY_STRING);
    if (!ready) {
        notification_others = notification_peer_unready(peer.id, EMPTY_STRING);
    }
    for (auto lobby_peer_id : lobby.peer_ids) {
        if (lobby_peer_id == peer.id) {
            continue;
        }
        send(game, lobby_peer_id, notification_others, uWS::OpCode::TEXT);
    }
    std::string notification_self = notification_peer_ready(peer.id, command_id);
    if (!ready) {
        notification_self = notification_peer_unready(peer.id, command_id);
    }
    send(game, peer.id, notification_self, uWS::OpCode::TEXT);
}

void GameThread::on_seal_lobby(GameData &game, std::string command_id, PeerData &peer,
                               yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_seal_lobby ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    if (lobby.host != peer.id) {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_HOST);
    }
    if (lobby.sealed) {
        return on_error(game, command_id, peer.id, ERROR_LOBBY_SEALED);
    }
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_can_seal") != game.enabled_callbacks.end()) {
        bool has_error = false;
        boost::container::vector<AnyElement> args(1);
        args[0] = AnyElement{true};
        auto func_result = scripted_function_call(peer.id, peer.lobby_id, game, "_can_seal", true,
                                                  args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(game, command_id, peer.id, std::get<std::string>(func_result.value),
                            false, has_error);
        }
    }
    set_lobby_sealed(game, lobby, peer.id, command_id, true);
}

void GameThread::on_unseal_lobby(GameData &game, std::string command_id, PeerData &peer,
                                 yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_unseal_lobby ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    if (lobby.host != peer.id) {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_HOST);
    }
    if (!lobby.sealed) {
        return on_error(game, command_id, peer.id, ERROR_LOBBY_NOT_SEALED);
    }
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_can_seal") != game.enabled_callbacks.end()) {
        bool has_error = false;
        boost::container::vector<AnyElement> args(1);
        args[0] = AnyElement{false};
        auto func_result = scripted_function_call(peer.id, peer.lobby_id, game, "_can_seal", true,
                                                  args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(game, command_id, peer.id, std::get<std::string>(func_result.value),
                            false, has_error);
        }
    }
    set_lobby_sealed(game, lobby, peer.id, command_id, false);
}

void GameThread::on_lobby_max_players(GameData &game, std::string command_id, PeerData &peer,
                                      yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_lobby_max_players ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    auto max_players = decode_int_or_default(data_val, "max_players", 0);
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_can_resize") != game.enabled_callbacks.end()) {
        bool has_error = false;
        boost::container::vector<AnyElement> args(1);
        args[0] = AnyElement{max_players};
        auto func_result = scripted_function_call(peer.id, peer.lobby_id, game, "_can_resize", true,
                                                  args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(game, command_id, peer.id, std::get<std::string>(func_result.value),
                            false, has_error);
        }
    }
    // CHANGES
    lobby.max_players = max_players;
    // NOTIFICATION
    std::string notification_others = notification_lobby_max_players(max_players, EMPTY_STRING);
    for (auto lobby_peer_id : lobby.peer_ids) {
        if (lobby_peer_id == peer.id && command_id != "") {
            continue;
        }
        send(game, lobby_peer_id, notification_others, uWS::OpCode::TEXT);
    }
    std::string notification_self = notification_lobby_max_players(max_players, command_id);
    send(game, peer.id, notification_self, uWS::OpCode::TEXT);
}

void GameThread::on_lobby_title(GameData &game, std::string command_id, PeerData &peer,
                                yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_lobby_title ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    auto title = decode_string_or_default(data_val, "name", "");
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_can_title") != game.enabled_callbacks.end()) {
        bool has_error = false;
        boost::container::vector<AnyElement> args(1);
        args[0] = AnyElement{title};
        auto func_result = scripted_function_call(peer.id, peer.lobby_id, game, "_can_title", true,
                                                  args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(game, command_id, peer.id, std::get<std::string>(func_result.value),
                            false, has_error);
        }
    }
    // CHANGES
    lobby.name = title;
    // NOTIFICATION
    std::string notification_others = notification_lobby_title(title, EMPTY_STRING);
    for (auto lobby_peer_id : lobby.peer_ids) {
        if (lobby_peer_id == peer.id && command_id != "") {
            continue;
        }
        send(game, lobby_peer_id, notification_others, uWS::OpCode::TEXT);
    }
    std::string notification_self = notification_lobby_title(title, command_id);
    send(game, peer.id, notification_self, uWS::OpCode::TEXT);
}

void GameThread::on_lobby_password(GameData &game, std::string command_id, PeerData &peer,
                                   yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_lobby_password ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    auto password = decode_string_or_default(data_val, "password", "");
    // CHANGES
    lobby.password = password;
    // NOTIFICATION
    std::string notification_others =
        notification_lobby_password_protected(password != "", EMPTY_STRING);
    for (auto lobby_peer_id : lobby.peer_ids) {
        if (lobby_peer_id == peer.id && command_id != "") {
            continue;
        }
        send(game, lobby_peer_id, notification_others, uWS::OpCode::TEXT);
    }
    std::string notification_self =
        notification_lobby_password_protected(password != "", command_id);
    send(game, peer.id, notification_self, uWS::OpCode::TEXT);
}

void GameThread::on_lobby_data(GameData &game, std::string command_id, PeerData &peer,
                               yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_lobby_data ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    if (lobby.host != peer.id) {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_HOST);
    }
    if (game.lobby_control != "relay") {
        return on_error(game, command_id, peer.id, ERROR_GAME_NOT_RELAY);
    }
    // CHANGES
    yyjson_val *lobbydata_val = yyjson_obj_get(data_val, "lobby_data");
    if (!lobbydata_val || !yyjson_is_obj(lobbydata_val)) {
        return on_error(game, command_id, peer.id,
                        ERROR_CANNOT_PARSE_JSON + " Lobby data missing or not object");
    }
    boost::container::flat_map<std::string, AnyElement> new_lobbydata;
    std::string error = decode_object(lobbydata_val, new_lobbydata);
    if (!error.empty()) {
        return on_error(game, command_id, peer.id, error);
    }
    bool is_private = decode_bool_or_default(data_val, "is_private", false);
    for (const auto &lobbydata_pair : new_lobbydata) {
        if (std::holds_alternative<std::monostate>(lobbydata_pair.second.value)) {
            if (is_private) {
                lobby.private_data_dirty = true;
                lobby.private_data.erase(lobbydata_pair.first);
            } else {
                lobby.public_data_dirty = true;
                lobby.public_data.erase(lobbydata_pair.first);
            }
            continue;
        }
        if (is_private) {
            lobby.private_data_dirty = true;
            lobby.private_data[lobbydata_pair.first] = lobbydata_pair.second;
        } else {
            lobby.public_data_dirty = true;
            lobby.public_data[lobbydata_pair.first] = lobbydata_pair.second;
        }
    }
    // NOTIFICATION
    if (lobby.public_data_dirty) {
        lobby.public_data_dirty = false;
        // Notify lobby public data update
        std::string public_data_notification_others =
            notification_lobby_public_data(AnyElement{lobby.public_data}, EMPTY_STRING);
        // send to others
        for (const auto &peer_id : lobby.peer_ids) {
            if (peer_id == peer.id) {
                continue;
            }
            send(game, peer_id, public_data_notification_others, uWS::OpCode::TEXT);
        }
        // send to self
        std::string public_data_notification_self =
            notification_lobby_public_data(AnyElement{lobby.public_data}, command_id);
        send(game, peer.id, public_data_notification_self, uWS::OpCode::TEXT);
    }
    if (lobby.private_data_dirty) {
        lobby.private_data_dirty = false;
        // Notify lobby private data update, send only to self
        std::string private_data_notification =
            notification_lobby_private_data(AnyElement{lobby.private_data}, command_id);
        send(game, peer.id, private_data_notification, uWS::OpCode::TEXT);
    }
}
void GameThread::on_data_to(GameData &game, std::string command_id, PeerData &peer,
                            yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_data_to ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    if (lobby.host != peer.id) {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_HOST);
    }
    if (game.lobby_control != "relay") {
        return on_error(game, command_id, peer.id, ERROR_GAME_NOT_RELAY);
    }
    std::string target_peer_id = decode_string_or_default(data_val, "target_peer", "");
    if (target_peer_id == "" || lobby.peer_ids.find(target_peer_id) == lobby.peer_ids.end()) {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_FOUND);
    }
    auto &target_peer = game.peers[target_peer_id];
    // CHANGES
    yyjson_val *peerdata_val = yyjson_obj_get(data_val, "peer_data");
    if (!peerdata_val || !yyjson_is_obj(peerdata_val)) {
        return on_error(game, command_id, peer.id,
                        ERROR_CANNOT_PARSE_JSON + " Peer data missing or not object");
    }
    boost::container::flat_map<std::string, AnyElement> new_data;
    std::string error = decode_object(peerdata_val, new_data);
    if (!error.empty()) {
        return on_error(game, command_id, peer.id, error);
    }
    bool is_private = decode_bool_or_default(data_val, "is_private", false);
    for (const auto &data_pair : new_data) {
        if (std::holds_alternative<std::monostate>(data_pair.second.value)) {
            if (is_private) {
                target_peer.private_data_dirty = true;
                target_peer.private_data.erase(data_pair.first);
            } else {
                target_peer.public_data_dirty = true;
                target_peer.public_data.erase(data_pair.first);
            }
            continue;
        }
        if (is_private) {
            target_peer.private_data_dirty = true;
            target_peer.private_data[data_pair.first] = data_pair.second;
        } else {
            target_peer.public_data_dirty = true;
            target_peer.public_data[data_pair.first] = data_pair.second;
        }
    }
    // NOTIFICATION
    if (target_peer.public_data_dirty) {
        target_peer.public_data_dirty = false;
        // Notify peer public data update
        std::string public_data_notification = notification_peer_public_data(
            AnyElement{target_peer.public_data}, peer.id, target_peer_id);
        // send to others
        for (const auto &peer_id : lobby.peer_ids) {
            send(game, peer_id, public_data_notification, uWS::OpCode::TEXT);
        }
    }
    if (target_peer.private_data_dirty) {
        target_peer.private_data_dirty = false;
        // Notify peer private data update, send only to self
        std::string private_data_notification = notification_peer_private_data(
            AnyElement{target_peer.private_data}, peer.id, target_peer_id);
        send(game, target_peer_id, private_data_notification, uWS::OpCode::TEXT);
    }
    // send to self
    std::string data_sent_notification = notification_data_to_sent(command_id);
    send(game, peer.id, data_sent_notification, uWS::OpCode::TEXT);
}
void GameThread::on_data_to_all(GameData &game, std::string command_id, PeerData &peer,
                                yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_data_to_all ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    if (lobby.host != peer.id) {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_HOST);
    }
    if (game.lobby_control != "relay") {
        return on_error(game, command_id, peer.id, ERROR_GAME_NOT_RELAY);
    }
    // CHANGES
    yyjson_val *peerdata_val = yyjson_obj_get(data_val, "peer_data");
    if (!peerdata_val || !yyjson_is_obj(peerdata_val)) {
        return on_error(game, command_id, peer.id,
                        ERROR_CANNOT_PARSE_JSON + " Peer data missing or not object");
    }
    boost::container::flat_map<std::string, AnyElement> new_data;
    std::string error = decode_object(peerdata_val, new_data);
    if (!error.empty()) {
        return on_error(game, command_id, peer.id, error);
    }
    bool is_private = decode_bool_or_default(data_val, "is_private", false);
    for (auto &peer_id : lobby.peer_ids) {
        auto &target_peer = game.peers[peer_id];

        for (const auto &data_pair : new_data) {
            if (std::holds_alternative<std::monostate>(data_pair.second.value)) {
                if (is_private) {
                    target_peer.private_data_dirty = true;
                    target_peer.private_data.erase(data_pair.first);
                } else {
                    target_peer.public_data_dirty = true;
                    target_peer.public_data.erase(data_pair.first);
                }
                continue;
            }
            if (is_private) {
                target_peer.private_data_dirty = true;
                target_peer.private_data[data_pair.first] = data_pair.second;
            } else {
                target_peer.public_data_dirty = true;
                target_peer.public_data[data_pair.first] = data_pair.second;
            }
        }
        // NOTIFICATION
        if (target_peer.public_data_dirty) {
            target_peer.public_data_dirty = false;
            // Notify peer public data update
            std::string public_data_notification = notification_peer_public_data(
                AnyElement{target_peer.public_data}, peer.id, target_peer.id);
            // send to others
            for (const auto &peer_id : lobby.peer_ids) {
                send(game, peer_id, public_data_notification, uWS::OpCode::TEXT);
            }
        }
        if (target_peer.private_data_dirty) {
            target_peer.private_data_dirty = false;
            // Notify peer private data update, send only to self
            std::string private_data_notification = notification_peer_private_data(
                AnyElement{target_peer.private_data}, peer.id, target_peer.id);
            send(game, target_peer.id, private_data_notification, uWS::OpCode::TEXT);
        }
    }
    // send to self
    std::string data_sent_notification = notification_data_to_sent(command_id);
    send(game, peer.id, data_sent_notification, uWS::OpCode::TEXT);
}
void GameThread::on_notify_to(GameData &game, std::string command_id, PeerData &peer,
                              yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_notify_to ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    if (lobby.host != peer.id) {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_HOST);
    }
    if (game.lobby_control != "relay") {
        return on_error(game, command_id, peer.id, ERROR_GAME_NOT_RELAY);
    }
    std::string target_peer_id = decode_string_or_default(data_val, "target_peer", "");
    if (target_peer_id == "" || lobby.peer_ids.find(target_peer_id) == lobby.peer_ids.end()) {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_FOUND);
    }
    auto &target_peer = game.peers[target_peer_id];
    // CHANGES
    yyjson_val *peerdata_val = yyjson_obj_get(data_val, "peer_data");
    if (!peerdata_val || !yyjson_is_obj(peerdata_val)) {
        return on_error(game, command_id, peer.id,
                        ERROR_CANNOT_PARSE_JSON + " Peer data missing or not object");
    }
    boost::container::flat_map<std::string, AnyElement> new_data;
    std::string error = decode_object(peerdata_val, new_data);
    if (!error.empty()) {
        return on_error(game, command_id, peer.id, error);
    }
    // NOTIFICATION
    // Notify peer data update, send only to target_peer
    std::string peer_data_notification = notification_peer_notify(AnyElement{new_data}, peer.id);
    send(game, target_peer_id, peer_data_notification, uWS::OpCode::TEXT);
    // send to self
    std::string data_sent_notification = notification_notify_sent(command_id);
    send(game, peer.id, data_sent_notification, uWS::OpCode::TEXT);
}
void GameThread::on_lobby_notify(GameData &game, std::string command_id, PeerData &peer,
                                 yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_lobby_notify ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(game, command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    if (game.lobby_control != "relay") {
        return on_error(game, command_id, peer.id, ERROR_GAME_NOT_RELAY);
    }
    // CHANGES
    yyjson_val *peerdata_val = yyjson_obj_get(data_val, "peer_data");
    if (!peerdata_val || !yyjson_is_obj(peerdata_val)) {
        return on_error(game, command_id, peer.id,
                        ERROR_CANNOT_PARSE_JSON + " Peer data missing or not object");
    }
    boost::container::flat_map<std::string, AnyElement> new_data;
    std::string error = decode_object(peerdata_val, new_data);
    if (!error.empty()) {
        return on_error(game, command_id, peer.id, error);
    }
    // NOTIFICATION
    // Notify peer notification update
    std::string peer_notification = notification_peer_notify(AnyElement{new_data}, peer.id);
    if (lobby.host == peer.id) {
        // if host, send to all
        for (auto &target_peer_id : lobby.peer_ids) {
            send(game, target_peer_id, peer_notification, uWS::OpCode::TEXT);
        }
    } else {
        // notify host
        send(game, lobby.host, peer_notification, uWS::OpCode::TEXT);
    }
    // Notify self
    std::string data_sent_notification = notification_notify_sent(command_id);
    send(game, peer.id, data_sent_notification, uWS::OpCode::TEXT);
}

void GameThread::set_lobby_sealed(GameData &game, LobbyData &lobby, std::string peer_id,
                                  std::string command_id, bool sealed) {
    // CHANGES
    lobby.sealed = sealed;
    // NOTIFICATION
    std::string notification_others = notification_lobby_unsealed(EMPTY_STRING);
    if (sealed) {
        notification_others = notification_lobby_sealed(EMPTY_STRING);
    }
    for (auto lobby_peer_id : lobby.peer_ids) {
        if (lobby_peer_id == peer_id) {
            continue;
        }
        send(game, lobby_peer_id, notification_others, uWS::OpCode::TEXT);
    }
    std::string notification_self = notification_lobby_unsealed(command_id);
    if (sealed) {
        notification_self = notification_lobby_sealed(command_id);
    }
    send(game, peer_id, notification_self, uWS::OpCode::TEXT);
}

void GameThread::reload_game(std::string folder_name) {
    for (auto &game_obj : games) {
        auto &game = game_obj.second;
        if (game.folder_name == folder_name) {
            if (game.lua.enabled) {
                game.close();
                game.open(get_time_now());
                if (game.enabled_callbacks.find("_on_reload") != game.enabled_callbacks.end()) {
                    bool has_error = false;
                    boost::container::vector<AnyElement> args;
                    auto func_result = scripted_function_call(EMPTY_STRING, EMPTY_STRING, game,
                                                              "_on_reload", true, args, has_error);
                    if (has_error && std::holds_alternative<std::string>(func_result.value)) {
                        logger.error_log("[GameThread] on_error _on_reload ",
                                         std::get<std::string>(func_result.value));
                    }
                }
            }
            if (game.angelscript.enabled) {
                game.close();
                game.open(get_time_now());
                if (game.enabled_callbacks.find("_on_reload") != game.enabled_callbacks.end()) {
                    bool has_error = false;
                    boost::container::vector<AnyElement> args;
                    auto func_result = scripted_function_call(EMPTY_STRING, EMPTY_STRING, game,
                                                              "_on_reload", true, args, has_error);
                    if (has_error && std::holds_alternative<std::string>(func_result.value)) {
                        logger.error_log("[GameThread] on_error _on_reload ",
                                         std::get<std::string>(func_result.value));
                    }
                }
            }
        }
    }
}

AnyElement GameThread::scripted_function_call(std::string peer_id, std::string lobby_id,
                                              GameData &game, std::string funcname, bool override,
                                              boost::container::vector<AnyElement> &args,
                                              bool &has_error) {
    if (game.lua.enabled) {
        auto result = game.lua.func_call(funcname, args, peer_id, lobby_id, game.id, has_error);
        // if dictionary with error, put error
        auto result_dict =
            std::get_if<boost::container::flat_map<std::string, AnyElement>>(&result.value);
        if (result_dict && result_dict->find("error") != result_dict->end()) {
            has_error = true;
            // logical error generates events too
            notify_lobby_changes(game, lobby_id);
            return (*result_dict)["error"];
        }
        notify_lobby_changes(game, lobby_id);
        return result;
    }
    if (game.angelscript.enabled) {
        auto result =
            game.angelscript.func_call(funcname, args, peer_id, lobby_id, game.id, has_error);
        // if dictionary with error, put error
        auto result_dict =
            std::get_if<boost::container::flat_map<std::string, AnyElement>>(&result.value);
        if (result_dict && result_dict->find("error") != result_dict->end()) {
            has_error = true;
            // logical error generates events too
            notify_lobby_changes(game, lobby_id);
            return (*result_dict)["error"];
        }
        notify_lobby_changes(game, lobby_id);
        return result;
    }
    return AnyElement{std::monostate{}};
}

void GameThread::send_message(GameData &game, std::string &lobby_id, std::string &message) {
    std::string notification = notification_chat(EMPTY_STRING, message, EMPTY_STRING);
    auto &lobby = game.lobbies[lobby_id];
    for (const auto &peer_id : lobby.peer_ids) {
        send(game, peer_id, notification, uWS::OpCode::TEXT);
    }
}

void GameThread::notify_lobby_changes(GameData &game, std::string &lobby_id) {
    if (lobby_id == "") {
        return;
    }
    auto &lobby = game.lobbies[lobby_id];

    for (const auto &peer_id : lobby.peer_ids) {
        auto &peer = game.peers[peer_id];
        if (peer.private_data_dirty) {
            peer.private_data_dirty = false;
            std::string private_data_notification = notification_peer_private_data(
                AnyElement{peer.private_data}, EMPTY_STRING, peer_id);
            // only send to self
            send(game, peer_id, private_data_notification, uWS::OpCode::TEXT);
        }
        if (peer.public_data_dirty) {
            peer.public_data_dirty = false;
            // Notify peer public data update
            std::string public_data_notification =
                notification_peer_public_data(AnyElement{peer.public_data}, EMPTY_STRING, peer_id);
            for (const auto &lobby_peer_id : lobby.peer_ids) {
                send(game, lobby_peer_id, public_data_notification, uWS::OpCode::TEXT);
            }
        }
    }

    if (lobby.tags_dirty) {
        lobby.tags_dirty = false;
        std::string tags_notification = notification_tags(AnyElement{lobby.tags}, EMPTY_STRING);
        for (auto &peer_id : lobby.peer_ids) {
            send(game, peer_id, tags_notification, uWS::OpCode::TEXT);
        }
    }

    if (lobby.name_dirty) {
        lobby.name_dirty = false;
        std::string name_notification = notification_lobby_title(lobby.name, EMPTY_STRING);
        for (auto &peer_id : lobby.peer_ids) {
            send(game, peer_id, name_notification, uWS::OpCode::TEXT);
        }
    }

    if (lobby.max_players_dirty) {
        lobby.max_players_dirty = false;
        std::string max_players_notification =
            notification_lobby_max_players(lobby.max_players, EMPTY_STRING);
        for (auto &peer_id : lobby.peer_ids) {
            send(game, peer_id, max_players_notification, uWS::OpCode::TEXT);
        }
    }

    if (lobby.sealed_dirty) {
        lobby.sealed_dirty = false;
        if (lobby.sealed) {
            std::string sealed_notification = notification_lobby_sealed(EMPTY_STRING);
            for (const auto &peer_id : lobby.peer_ids) {
                send(game, peer_id, sealed_notification, uWS::OpCode::TEXT);
            }
        } else {
            std::string unsealed_notification = notification_lobby_unsealed(EMPTY_STRING);
            for (const auto &peer_id : lobby.peer_ids) {
                send(game, peer_id, unsealed_notification, uWS::OpCode::TEXT);
            }
        }
    }

    if (lobby.public_data_dirty) {
        lobby.public_data_dirty = false;
        // Notify lobby public data update
        std::string public_data_notification =
            notification_lobby_public_data(AnyElement{lobby.public_data}, EMPTY_STRING);
        for (const auto &peer_id : lobby.peer_ids) {
            send(game, peer_id, public_data_notification, uWS::OpCode::TEXT);
        }
    }
    // DO NOT SEND PRIVATE DATA FOR SCRIPTED LOBBY
    if (lobby.private_data_dirty && game.lobby_control == "relay") {
        lobby.private_data_dirty = false;
        // Notify lobby private data update
        std::string private_data_notification =
            notification_lobby_private_data(AnyElement{lobby.private_data}, EMPTY_STRING);
        for (const auto &peer_id : lobby.peer_ids) {
            send(game, peer_id, private_data_notification, uWS::OpCode::TEXT);
        }
    }
}

void GameThread::notify_peer(GameData &game, std::string &lobby_id, std::string peer_id,
                             const AnyElement &notification) {
    auto &lobby = game.lobbies[lobby_id];
    std::string notification_message = notification_peer_notify(notification, peer_id);
    send(game, peer_id, notification_message, uWS::OpCode::TEXT);
}
