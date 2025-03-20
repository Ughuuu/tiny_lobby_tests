#include "game_thread.h"

#include <regex>

#include "INIReader.h"
#include "any_type.h"
#include "lua.h"

std::string ERROR_CANNOT_PARSE_JSON = "Cannot parse json";
std::string ERROR_PEER_NOT_FOUND = "Peer not found";
std::string ERROR_PEER_ALREADY_EXISTS = "Peer already exists";
std::string ERROR_PEER_NOT_IN_A_LOBBY = "Peer not in a lobby";
std::string ERROR_CANNOT_KICK_SELF = "Cannot kick self";
std::string ERROR_INVALID_FUNCTION = "Invalid function";
std::string ERROR_INVALID_ARGUMENTS = "Invalid arguments";
std::string ERROR_PEER_IS_IN_A_LOBBY = "Peer is in a lobby";
std::string ERROR_PEER_NOT_HOST = "Peer is not the host";
std::string ERROR_PEER_IS_READY = "Peer is ready";
std::string ERROR_PEER_IS_NOT_READY = "Peer is not ready";
std::string ERROR_LOBBY_NOT_SEALED = "Lobby is not sealed";
std::string ERROR_LOBBY_SEALED = "Lobby is sealed";
std::string ERROR_LOBBY_NOT_FOUND = "Lobby not found";
std::string ERROR_GAME_NOT_FOUND = "Game not found";
std::string ERROR_UNKOWN_COMMAND = "Unkown command";

std::string NOTIFICATION_ERROR =
    "{"
    "\"command\": \"error\","
    "\"message\": \"%s\","
    "\"data\": {"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_LOGICAL_ERROR =
    "{"
    "\"command\": \"error\","
    "\"message\": \"%s\","
    "\"data\": {"
    "\"id\": \"%s\""
    "},"
    "\"is_logical_error\": true"
    "}";
std::string NOTIFICATION_LOBBY_CREATED =
    "{"
    "\"command\": \"lobby_created\","
    "\"message\": \"Lobby created\","
    "\"data\": {"
    "\"lobby\": %s,"
    "\"peers\": %s,"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_LOBBY_UNSEALED =
    "{"
    "\"command\": \"lobby_unsealed\","
    "\"message\": \"Lobby unsealed\","
    "\"data\": {"
    "\"peer_id\": \"%s\","
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_LOBBY_SEALED =
    "{"
    "\"command\": \"lobby_sealed\","
    "\"message\": \"Lobby sealed\","
    "\"data\": {"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_PEER_READY =
    "{"
    "\"command\": \"peer_ready\","
    "\"message\": \"Peer ready\","
    "\"data\": {"
    "\"peer_id\": \"%s\","
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_PEER_UNREADY =
    "{"
    "\"command\": \"peer_unready\","
    "\"message\": \"Peer unready\","
    "\"data\": {"
    "\"peer_id\": \"%s\","
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_LOBBY_LEFT =
    "{"
    "\"command\": \"lobby_left\","
    "\"message\": \"Lobby Left\","
    "\"data\": {"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_PEER_LEFT =
    "{"
    "\"command\": \"peer_left\","
    "\"message\": \"Peer left\","
    "\"data\": {"
    "\"peer_id\": \"%s\","
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_PEER_KICKED =
    "{"
    "\"command\": \"peer_left\","
    "\"message\": \"Peer kicked\","
    "\"data\": {"
    "\"peer_id\": \"%s\","
    "\"kicked\": true,"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_LOBBY_KICKED =
    "{"
    "\"command\": \"lobby_kicked\","
    "\"message\": \"Lobby kicked\""
    "}";
std::string NOTIFICATION_PEER_STATE =
    "{"
    "\"command\": \"peer_state\","
    "\"message\": \"Initial Message\","
    "\"data\": {"
    "\"peer\": %s"
    "}"
    "}";
std::string NOTIFICATION_LOBBY_CALL =
    "{"
    "\"command\": \"lobby_call\","
    "\"message\": \"Lobby Call\","
    "\"data\": {"
    "\"result\": %s,"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_USER_DATA =
    "{"
    "\"command\": \"peer_user_data\","
    "\"message\": \"User Data\","
    "\"data\": {"
    "\"user_data\": %s,"
    "\"peer_id\": \"%s\","
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_TAGS =
    "{"
    "\"command\": \"lobby_tags\","
    "\"message\": \"Tags Set\","
    "\"data\": {"
    "\"tags\": %s,"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_CHAT =
    "{"
    "\"command\": \"peer_chat\","
    "\"message\": \"Chat\","
    "\"data\": {"
    "\"from_peer\": \"%s\","
    "\"chat_data\": \"%s\","
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_PEER_RECONNECTED =
    "{"
    "\"command\": \"peer_reconnected\","
    "\"message\": \"Peer reconnected\","
    "\"data\": {"
    "\"peer_id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_LOBBY_JOINED =
    "{"
    "\"command\": \"joined_lobby\","
    "\"message\": \"Lobby joined\","
    "\"data\": {"
    "\"lobby\": %s,"
    "\"peers\": %s,"
    "\"id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_PEER_JOINED =
    "{"
    "\"command\": \"peer_joined\","
    "\"message\": \"Peer joined\","
    "\"data\": {"
    "\"peer\": %s"
    "}"
    "}";
std::string NOTIFICATION_PEER_DICONNECTED =
    "{"
    "\"command\": \"peer_disconnected\","
    "\"message\": \"Peer disconnected\","
    "\"data\": {"
    "\"peer_id\": \"%s\""
    "}"
    "}";
std::string NOTIFICATION_LOBBY_LIST =
    "{"
    "\"command\": \"lobby_list\","
    "\"message\": \"List Lobbies\","
    "\"data\": {"
    "\"lobbies\": %s,"
    "\"id\": \"%s\""
    "}"
    "}";

GameThread::GameThread(bool verbose, std::string log_folder, std::string scripts_folder,
                       moodycamel::BlockingReaderWriterQueue<WebSocketMessage> &message_queue,
                       uWS::Loop *loop, WebSocketServer<true> *webserver,
                       WebSocketServer<false> *webserver_no_ssl)
    : logger(verbose, log_folder + "/game.txt"),
      scripts_folder(scripts_folder),
      message_queue(message_queue),
      loop(loop),
      webserver_ssl(webserver),
      webserver_nossl(webserver_no_ssl),
      logs_folder(log_folder) {
    logger.debug_log("[GameThread] on_start");
}

void GameThread::load_games() {
    logger.debug_log("[GameThread] on_load_games");
    if (!std::filesystem::exists(scripts_folder)) {
        logger.error_log("[GameThread] Cannot open " + scripts_folder);
        return;
    }
    for (const auto &entry : std::filesystem::directory_iterator(scripts_folder)) {
        if (entry.is_directory()) {
            std::string folder_name = entry.path().filename().string();
            logger.debug_log("[GameThread] on_load_game ", folder_name);
            INIReader config_reader(scripts_folder + "/" + folder_name + "/config.ini");
            if (config_reader.ParseError() < 0) {
                logger.debug_log("[GameThread] Cannot open " + scripts_folder + "/" + folder_name +
                                 "/config.ini");
            }
            std::string script_language = config_reader.Get("game", "language", "lua");
            if (script_language != "lua" && script_language != "angelscript") {
                logger.error_log("[GameThread] Unknown script language: " + script_language);
                continue;
            }
            std::string script_entrypoint = config_reader.Get("game", "entrypoint", "");
            if (script_language == "lua" && script_entrypoint == "") {
                script_entrypoint = "main.lua";
            }
            if (script_language == "angelscript" && script_entrypoint == "") {
                script_entrypoint = "main.as";
            }
            std::unordered_set<std::string> enabled_callbacks;
            if (config_reader.GetBoolean("script", "callback_on_create", false)) {
                enabled_callbacks.insert("_on_create");
            }
            if (config_reader.GetBoolean("script", "callback_on_join", false)) {
                enabled_callbacks.insert("_on_join");
            }
            if (config_reader.GetBoolean("script", "callback_on_chat", false)) {
                enabled_callbacks.insert("_on_chat");
            }
            if (config_reader.GetBoolean("script", "callback_on_tags", false)) {
                enabled_callbacks.insert("_on_tags");
            }
            if (config_reader.GetBoolean("script", "callback_on_kick", false)) {
                enabled_callbacks.insert("_on_kick");
            }
            if (config_reader.GetBoolean("script", "callback_on_ready", false)) {
                enabled_callbacks.insert("_on_ready");
            }
            if (config_reader.GetBoolean("script", "callback_on_seal", false)) {
                enabled_callbacks.insert("_on_seal");
            }
            if (config_reader.GetBoolean("script", "callback_on_left", false)) {
                enabled_callbacks.insert("_on_left");
            }
            long tickrate = config_reader.GetInteger("game", "tickrate", -1);
            games.emplace(folder_name,
                          GameData{
                              .id = folder_name,
                              .entrypoint = script_entrypoint,
                              .tick_rate = tickrate,
                              .enabled_callbacks = enabled_callbacks,
                              .lua =
                                  ScriptLua{
                                      .script_language = script_language,
                                      .scripts_folder = scripts_folder,
                                      .folder_name = folder_name,
                                      .script_entrypoint = script_entrypoint,
                                      .logs_folder = logs_folder,
                                      .game_thread = this,
                                  },
                              //.angelscript = ScriptAngelScript(script_language, scripts_folder,
                              // folder_name, script_entrypoint, logger, config_reader)
                          });
            games[folder_name].open();
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

void GameThread::run() {
    load_games();
    logger.debug_log("[GameThread] on_start_thread");
    int64_t last_listing = get_time_now();
    int64_t last_disconnect = get_time_now();
    int64_t last_timers = get_time_now();
    int64_t listing_interval = LISTING_INTERVAL;
    int64_t disconnect_interval = MAX_RECONNECTION_TIME / 6;
    int64_t timer_interval = 500;
    while (true) {
        int64_t now = get_time_now();
        handle_events();
        if (now - last_disconnect > disconnect_interval) {
            last_disconnect = now;
            handle_disconnects();
        }
        if (now - last_listing > listing_interval) {
            std::cout << now << " , " << messages_received << " , " << messages_sent << " , "
                      << message_queue.size_approx() << std::endl;
            messages_received = 0;
            messages_sent = 0;
            last_listing = now;
            handle_lobby_list();
        }
        if (now - last_timers > timer_interval) {
            last_timers = now;
            handle_timers();
        }
        // handle tick if/when needed
    }
    unload_games();
}

void GameThread::handle_timers() {
    int64_t last_listing = get_time_now();
    for (auto &game : games) {
        for (auto &timer_data : game.second.timer_data) {
            if (timer_data.second.end_time < last_listing) {
                bool has_error = false;
                auto result = scripted_function_call(timer_data.second.peer_id, timer_data.second.lobby_id, game.second,
                                                     timer_data.second.id, true,
                                                     timer_data.second.args, has_error);
                if (has_error) {
                    on_error(EMPTY_STRING, timer_data.second.peer_id, std::get<std::string>(result.value), true);
                }
                game.second.timer_data.erase(timer_data.first);
            }
        }
    }
}

void GameThread::handle_events() {
    WebSocketMessage message;
    if (!message_queue.wait_dequeue_timed(message, 50)) {
        return;
    }
    messages_received++;
    if (games.find(message.game_id) == games.end()) {
        on_error(EMPTY_STRING, message.id, ERROR_GAME_NOT_FOUND + message.game_id, true);
        return;
    }
    auto &game = games[message.game_id];
    switch (message.event) {
        case WebSocketEvent::OPEN:
            on_connect(game, message.id, message.game_id, message.reconnection_token);
            break;
        case WebSocketEvent::CLOSE:
            on_close(game, message.id);
            break;
        case WebSocketEvent::MESSAGE:
            // decode message
            yyjson_doc *doc = yyjson_read(message.message.c_str(), message.message.size(), 0);
            if (!doc) {
                on_error(EMPTY_STRING, message.id, ERROR_CANNOT_PARSE_JSON + " Initial decode",
                         true);
                break;
            }

            yyjson_val *root = yyjson_doc_get_root(doc);
            if (!root || !yyjson_is_obj(root)) {
                yyjson_doc_free(doc);
                on_error(EMPTY_STRING, message.id, ERROR_CANNOT_PARSE_JSON + " Root is not object",
                         true);
                break;
            }
            // get command
            yyjson_val *command_val = yyjson_obj_get(root, "command");
            if (!command_val || !yyjson_is_str(command_val)) {
                yyjson_doc_free(doc);
                on_error(EMPTY_STRING, message.id,
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
                on_error(std::string(command_id), message.id, ERROR_PEER_NOT_FOUND, true);
                break;
            }
            auto &peer = game.peers[message.id];
            if (command == "lobby_call") {
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
            } else {
                on_error(std::string(command_id), message.id,
                         ERROR_UNKOWN_COMMAND + " " + std::string(command), true);
            }
            yyjson_doc_free(doc);
            break;
    }
}

void GameThread::handle_disconnects() {
    for (auto &game : games) {
        auto &game_data = game.second;
        auto now = get_time_now();
        std::unordered_set<std::string> to_erase;
        for (auto &peer : game_data.disconnected_peers) {
            auto &peer_obj = game_data.peers[peer.first];
            // peer no longer in lobby
            if (peer_obj.lobby_id == EMPTY_STRING) {
                peer_obj.leave_lobby();
                to_erase.insert(peer.first);
                continue;
            }
            auto &lobby = game_data.lobbies[peer_obj.lobby_id];
            if (now - peer.second > MAX_RECONNECTION_TIME) {
                remove_peer_from_lobby(game_data, lobby, peer_obj, EMPTY_STRING);
                to_erase.insert(peer.first);
            }
        }
        for (auto &peer_id : to_erase) {
            game_data.disconnected_peers.erase(peer_id);
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
        std::string lobbies = "[";
        for (const auto &lobby_id : game_data.lobbies_updated) {
            // if deleted, only send id
            if (game_data.lobbies.find(lobby_id) == game_data.lobbies.end()) {
                lobbies += "{\"id\":\"" + lobby_id + "\"},";
                continue;
            }
            auto &lobby = game_data.lobbies[lobby_id];
            lobbies += lobby.to_string() + ",";
        }
        if (!game_data.lobbies_updated.empty()) {
            lobbies = lobbies.substr(0, lobbies.size() - 1);
        }
        lobbies += "]";

        std::string notification = NOTIFICATION_LOBBY_LIST;
        notification.replace(notification.find("%s"), 2, lobbies);
        notification.replace(notification.find("%s"), 2, EMPTY_STRING);
        for (auto &peer : game_data.lobby_listing_peers) {
            send(peer, notification, uWS::OpCode::TEXT);
        }
        game_data.lobbies_updated.clear();
    }
}

void GameThread::remove_peer_from_lobby(GameData &game, LobbyData &lobby, PeerData &peer,
                                        const std::string &command_id) {
    if (game.enabled_callbacks.find("_on_left") != game.enabled_callbacks.end()) {
        bool has_error = false;
        std::vector<AnyElement> args;
        auto func_result =
            scripted_function_call(peer.id, peer.lobby_id, game, "_on_left", true, args, has_error);
        if (has_error && std::holds_alternative<std::string>(func_result.value)) {
            return on_error(command_id, peer.id, std::get<std::string>(func_result.value), false,
                            has_error);
        }
    }
    peer.ready = false;
    if (peer.id == lobby.host) {
        std::string notification_others = NOTIFICATION_LOBBY_LEFT;
        notification_others.replace(notification_others.find("%s"), 2, EMPTY_STRING);
        for (auto lobby_peer_id : lobby.peer_ids) {
            auto &lobby_peer = game.peers[lobby_peer_id];
            lobby_peer.leave_lobby();
            if (lobby_peer_id == peer.id) {
                continue;
            }
            send(lobby_peer_id, notification_others, uWS::OpCode::TEXT);
        }
        std::string notification_self = NOTIFICATION_LOBBY_LEFT;
        notification_self.replace(notification_self.find("%s"), 2, command_id);
        send(peer.id, notification_self, uWS::OpCode::TEXT);
        game.lobbies_updated.insert(lobby.id);
        game.lobbies.erase(lobby.id);
    } else {
        std::string notification_others = NOTIFICATION_PEER_LEFT;
        notification_others.replace(notification_others.find("%s"), 2, peer.id);
        notification_others.replace(notification_others.find("%s"), 2, EMPTY_STRING);
        for (auto lobby_peer_id : lobby.peer_ids) {
            auto &lobby_peer = game.peers[lobby_peer_id];
            lobby_peer.leave_lobby();
            if (lobby_peer_id == peer.id) {
                continue;
            }
            send(lobby_peer_id, notification_others, uWS::OpCode::TEXT);
        }
        std::string notification_self = NOTIFICATION_LOBBY_LEFT;
        notification_self.replace(notification_self.find("%s"), 2, command_id);
        lobby.peer_ids.erase(peer.id);
        send(peer.id, notification_self, uWS::OpCode::TEXT);
    }
}

void GameThread::send(const std::string &peer_id, const std::string &message, uWS::OpCode opCode) {
    messages_sent++;
    if (webserver_ssl != nullptr) {
        loop->defer([id = peer_id, msg = message, webserver = webserver_ssl]() {
            webserver->send(id, msg, uWS::OpCode::TEXT);
        });
    } else {
        loop->defer([id = peer_id, msg = message, webserver = webserver_nossl]() {
            webserver->send(id, msg, uWS::OpCode::TEXT);
        });
    }
}

void GameThread::on_connect(GameData &game, std::string &peer_id, std::string &game_id,
                            std::string &reconnection_token) {
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
                                    });
    }
    auto &peer = game.peers[peer_id];
    std::string notification = NOTIFICATION_PEER_STATE;
    notification.replace(notification.find("%s"), 2, peer.to_string(true, true));
    send(peer.id, notification, uWS::OpCode::TEXT);
}

void GameThread::on_close(GameData &game, std::string &peer_id) {
    logger.debug_log("[GameThread] on_close ", peer_id);
    auto &peer = game.peers[peer_id];
    peer.ready = false;
    peer.disconnected = true;
    game.disconnected_peers.emplace(peer_id, get_time_now());
    std::string notification = NOTIFICATION_PEER_DICONNECTED;
    notification.replace(notification.find("%s"), 2, peer_id);
    if (game.lobbies.find(peer.lobby_id) != game.lobbies.end()) {
        auto &lobby = game.lobbies[peer.lobby_id];
        for (auto lobby_peer_id : lobby.peer_ids) {
            if (lobby_peer_id == peer_id) {
                continue;
            }
            send(lobby_peer_id, notification, uWS::OpCode::TEXT);
        }
    }
}

void GameThread::on_error(std::string command_id, std::string peer_id, std::string message,
                          bool close, bool logical_error) {
    if (close) {
        logger.error_log("[GameThread] on_error ", command_id, " ", peer_id, " ", message);
    }
    std::string result = NOTIFICATION_ERROR;
    if (logical_error) {
        result = NOTIFICATION_LOGICAL_ERROR;
    }
    result.replace(result.find("%s"), 2, message);
    result.replace(result.find("%s"), 2, command_id);
    if (close) {
        send(peer_id, message, uWS::OpCode::CLOSE);
    } else {
        send(peer_id, result, uWS::OpCode::TEXT);
    }
}

void GameThread::on_lobby_call(GameData &game, std::string command_id, PeerData &peer,
                               yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_lobby_call ", command_id, " ", peer.id);
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto func_name = decode_string_or_default(data_val, "function", "");
    if (func_name == "") {
        return on_error(command_id, peer.id, ERROR_INVALID_FUNCTION);
    }
    AnyElement args{std::vector<AnyElement>()};
    yyjson_val *inputs_val = yyjson_obj_get(data_val, "inputs");
    if (inputs_val && yyjson_is_arr(inputs_val)) {
        decode_array(inputs_val, args);
    }
    if (!std::holds_alternative<std::vector<AnyElement>>(args.value)) {
        return on_error(command_id, peer.id, ERROR_INVALID_ARGUMENTS);
    }
    auto &array_value = std::get<std::vector<AnyElement>>(args.value);
    // SCRIPTED CALL
    bool has_error = false;
    auto func_result =
        scripted_function_call(peer.id, peer.lobby_id, game, func_name, true, array_value, has_error);
    if (has_error && std::holds_alternative<std::string>(func_result.value)) {
        return on_error(command_id, peer.id, std::get<std::string>(func_result.value), false,
                        has_error);
    } else {
        std::string notification = NOTIFICATION_LOBBY_CALL;
        notification.replace(notification.find("%s"), 2, func_result.to_string());
        notification.replace(notification.find("%s"), 2, command_id);
        send(peer.id, notification, uWS::OpCode::TEXT);
    }
}

void GameThread::on_quick_join(GameData &game, std::string command_id, PeerData &peer,
                               yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_quick_join ", command_id, " ", peer.id);
    if (peer.lobby_id != "") {
        if (on_join_lobby(game, command_id, peer, data_val)) {
            return;
        }
    }
    // TODO optimization keep lobbies open
    for (auto &lobby_obj : game.lobbies) {
        auto &lobby = lobby_obj.second;
        if (lobby.max_players > lobby.peer_ids.size() &&
            !lobby.sealed && !lobby.password.size() &&
            !game.peers[lobby.host].disconnected) {
            if (on_join_lobby(game, command_id, peer, data_val, lobby.id)) {
                return;
            }
        }
    }
    on_create_lobby(game, command_id, peer, data_val);
}

void GameThread::on_create_lobby(GameData &game, std::string command_id, PeerData &peer,
                                 yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_create_lobby ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id != "") {
        return on_error(command_id, peer.id, ERROR_PEER_IS_IN_A_LOBBY + " " + peer.lobby_id);
    }
    // CHANGES before scripted call, reverted if scripted call fails
    auto uuid = to_string(gen());
    auto small_uuid = uuid.substr(0, 8);
    std::unordered_map<std::string, AnyElement> lobby_tags;
    std::string decode_error = decode_object(yyjson_obj_get(data_val, "tags"), lobby_tags);
    if (decode_error != "") {
        return on_error(command_id, peer.id, "Invalid tags " + decode_error);
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
                             .create_time = get_time_now(),
                             .game_id = peer.game_id,
                             .tags = lobby_tags,
                         });
    game.lobby_listing_peers.erase(peer.id);
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_on_create") != game.enabled_callbacks.end()) {
        bool has_error = false;
        std::vector<AnyElement> args;
        auto func_result =
            scripted_function_call(peer.id, peer.lobby_id, game, "_on_create", true, args, has_error);
        if (has_error && std::holds_alternative<std::string>(func_result.value)) {
            // revert the changes
            peer.leave_lobby();
            game.lobbies.erase(small_uuid);
            return on_error(command_id, peer.id, std::get<std::string>(func_result.value), false,
                            has_error);
        }
    }
    game.lobbies_updated.insert(peer.lobby_id);
    peer.disconnected = false;
    // NOTIFICATION
    std::string notification = NOTIFICATION_LOBBY_CREATED;
    notification.replace(notification.find("%s"), 2, game.lobbies[small_uuid].to_string());
    notification.replace(notification.find("%s"), 2, game.peers_to_string(small_uuid));
    notification.replace(notification.find("%s"), 2, command_id);
    send(peer.id, notification, uWS::OpCode::TEXT);
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
        on_error(command_id, peer.id, "Invalid lobby id");
        return false;
    }
    auto &lobby = game.lobbies[lobby_id];
    if (!reconnecting) {
        if (peer.lobby_id != "") {
            on_error(command_id, peer.id, ERROR_PEER_IS_IN_A_LOBBY);
            return false;
        }
        if (lobby.password != decode_string_or_default(data_val, "password", "")) {
            on_error(command_id, peer.id, "Invalid password");
            return false;
        }
        if (lobby.max_players != 0 && lobby.peer_ids.size() >= lobby.max_players) {
            on_error(command_id, peer.id, "Lobby is full");
            return false;
        }
    }
    // SCRIPTED CALL
    if (!reconnecting) {
        if (game.enabled_callbacks.find("_on_join") != game.enabled_callbacks.end()) {
            bool has_error = false;
            std::vector<AnyElement> args;
            auto func_result =
                scripted_function_call(peer.id, lobby_id, game, "_on_join", true, args, has_error);
            if (has_error && std::holds_alternative<std::string>(func_result.value)) {
                on_error(command_id, peer.id, std::get<std::string>(func_result.value), false,
                         has_error);
                return false;
            }
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
        std::string notification = NOTIFICATION_PEER_RECONNECTED;
        notification.replace(notification.find("%s"), 2, peer.id);
        for (auto lobby_peer_id : lobby.peer_ids) {
            if (lobby_peer_id == peer.id) {
                continue;
            }
            send(lobby_peer_id, notification, uWS::OpCode::TEXT);
        }
    } else {
        std::string notification_others = NOTIFICATION_PEER_JOINED;
        notification_others.replace(notification_others.find("%s"), 2, peer.to_string());
        for (auto lobby_peer_id : lobby.peer_ids) {
            if (lobby_peer_id == peer.id) {
                continue;
            }
            send(lobby_peer_id, notification_others, uWS::OpCode::TEXT);
        }
    }
    std::string notification_self = NOTIFICATION_LOBBY_JOINED;
    notification_self.replace(notification_self.find("%s"), 2, lobby.to_string());
    notification_self.replace(notification_self.find("%s"), 2, game.peers_to_string(lobby.id));
    notification_self.replace(notification_self.find("%s"), 2, command_id);
    send(peer.id, notification_self, uWS::OpCode::TEXT);
    return true;
}

void GameThread::on_leave_lobby(GameData &game, std::string command_id, PeerData &peer,
                                yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_leave_lobby ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    // CHANGES
    remove_peer_from_lobby(game, lobby, peer, command_id);
}

void GameThread::on_list_lobby(GameData &game, std::string command_id, PeerData &peer,
                               yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_list_lobby ", command_id, " ", peer.id);
    if (game.lobby_listing_peers.find(peer.id) != game.lobby_listing_peers.end()) {
        std::string notification = NOTIFICATION_LOBBY_LIST;
        notification.replace(notification.find("%s"), 2, "[]");
        notification.replace(notification.find("%s"), 2, command_id);
        return send(peer.id, notification, uWS::OpCode::TEXT);
    }

    game.lobby_listing_peers.insert(peer.id);

    std::vector<LobbyData *> lobby_array;
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
    std::vector<std::string> selected_lobbies;
    for (int i = 0; i < max_lobbies_to_get; ++i) {
        selected_lobbies.push_back(lobby_array[i]->id);
    }

    // Create lobby objects
    std::string lobbies = "[";
    for (const auto &lobby_id : selected_lobbies) {
        auto &lobby = game.lobbies[lobby_id];
        lobbies += lobby.to_string() + ",";
    }
    if (!selected_lobbies.empty()) {
        lobbies = lobbies.substr(0, lobbies.size() - 1);
    }
    lobbies += "]";

    std::string notification = NOTIFICATION_LOBBY_LIST;
    notification.replace(notification.find("%s"), 2, lobbies);
    notification.replace(notification.find("%s"), 2, command_id);
    send(peer.id, notification, uWS::OpCode::TEXT);
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
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    std::string message = decode_string_or_default(data_val, "chat", "");
    message = strip_BBCode(message);
    if (message == "" || message.size() > 256) {
        return on_error(command_id, peer.id, "Invalid chat message");
    }
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_on_chat") != game.enabled_callbacks.end()) {
        bool has_error = false;
        std::vector<AnyElement> args(1);
        args[0] = AnyElement{message};
        auto func_result =
            scripted_function_call(peer.id, peer.lobby_id, game, "_on_chat", true, args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(command_id, peer.id, std::get<std::string>(func_result.value), false,
                            has_error);
        }
    }
    // NOTIFICATION
    std::string notification_others = NOTIFICATION_CHAT;
    notification_others.replace(notification_others.find("%s"), 2, peer.id);
    notification_others.replace(notification_others.find("%s"), 2, message);
    notification_others.replace(notification_others.find("%s"), 2, EMPTY_STRING);
    for (auto lobby_peer_id : game.lobbies[peer.lobby_id].peer_ids) {
        if (lobby_peer_id == peer.id) {
            continue;
        }
        send(lobby_peer_id, notification_others, uWS::OpCode::TEXT);
    }
    std::string notification_self = NOTIFICATION_CHAT;
    notification_self.replace(notification_self.find("%s"), 2, peer.id);
    notification_self.replace(notification_self.find("%s"), 2, message);
    notification_self.replace(notification_self.find("%s"), 2, command_id);
    send(peer.id, notification_self, uWS::OpCode::TEXT);
}

void GameThread::on_lobby_tags(GameData &game, std::string command_id, PeerData &peer,
                               yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_lobby_tags ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    if (lobby.host != peer.id) {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_HOST);
    }
    std::unordered_map<std::string, AnyElement> new_tags;
    if (!data_val || !yyjson_is_obj(data_val)) {
        return on_error(command_id, peer.id,
                        ERROR_CANNOT_PARSE_JSON + " Data is missing or not an object");
    }
    yyjson_val *tags_val = yyjson_obj_get(data_val, "tags");
    if (!tags_val || !yyjson_is_obj(tags_val)) {
        return on_error(command_id, peer.id,
                        ERROR_CANNOT_PARSE_JSON + " Lobby tags missing or not object");
    }
    std::string error = decode_object(tags_val, new_tags);
    if (!error.empty()) {
        return on_error(command_id, peer.id, error);
    }
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_on_tags") != game.enabled_callbacks.end()) {
        bool has_error = false;
        std::vector<AnyElement> args(1);
        args[0] = AnyElement{new_tags};
        auto func_result =
            scripted_function_call(peer.id, peer.lobby_id, game, "_on_tags", true, args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(command_id, peer.id, std::get<std::string>(func_result.value), false,
                            has_error);
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
    std::string tags_string = AnyElement{lobby.tags}.to_string();
    std::string notification_others = NOTIFICATION_TAGS;
    notification_others.replace(notification_others.find("%s"), 2, tags_string);
    notification_others.replace(notification_others.find("%s"), 2, EMPTY_STRING);
    for (auto lobby_peer_id : game.lobbies[peer.lobby_id].peer_ids) {
        if (lobby_peer_id == peer.id) {
            continue;
        }
        send(lobby_peer_id, notification_others, uWS::OpCode::TEXT);
    }
    std::string notification_self = NOTIFICATION_TAGS;
    notification_self.replace(notification_self.find("%s"), 2, tags_string);
    notification_self.replace(notification_self.find("%s"), 2, command_id);
    send(peer.id, notification_self, uWS::OpCode::TEXT);
}

void GameThread::on_kick_peer(GameData &game, std::string command_id, PeerData &peer,
                              yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_kick_peer ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    if (lobby.host != peer.id) {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_HOST);
    }
    std::string kicked_peer_id = decode_string_or_default(data_val, "peer_id", "");
    if (kicked_peer_id == "" || lobby.peer_ids.find(kicked_peer_id) == lobby.peer_ids.end()) {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    if (peer.id == kicked_peer_id) {
        return on_error(command_id, peer.id, ERROR_CANNOT_KICK_SELF);
    }
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_on_kick") != game.enabled_callbacks.end()) {
        bool has_error = false;
        std::vector<AnyElement> args(1);
        args[0] = AnyElement{kicked_peer_id};
        auto func_result =
            scripted_function_call(peer.id, peer.lobby_id, game, "_on_kick", true, args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(command_id, peer.id, std::get<std::string>(func_result.value), false,
                            has_error);
        }
    }
    // CHANGES
    lobby.peer_ids.erase(kicked_peer_id);
    auto &kicked_peer = game.peers[kicked_peer_id];
    kicked_peer.leave_lobby();
    // NOTIFICATION
    std::string notification_others = NOTIFICATION_PEER_KICKED;
    notification_others.replace(notification_others.find("%s"), 2, kicked_peer_id);
    notification_others.replace(notification_others.find("%s"), 2, EMPTY_STRING);
    for (auto lobby_peer_id : game.lobbies[lobby.id].peer_ids) {
        if (lobby_peer_id == peer.id || lobby_peer_id == kicked_peer_id) {
            continue;
        }
        send(lobby_peer_id, notification_others, uWS::OpCode::TEXT);
    }
    std::string notification_self = NOTIFICATION_PEER_KICKED;
    notification_self.replace(notification_self.find("%s"), 2, kicked_peer_id);
    notification_self.replace(notification_self.find("%s"), 2, command_id);
    send(peer.id, notification_self, uWS::OpCode::TEXT);
    std::string notification_kicked = NOTIFICATION_LOBBY_KICKED;
    send(kicked_peer_id, notification_kicked, uWS::OpCode::TEXT);
}

void GameThread::on_user_data(GameData &game, std::string command_id, PeerData &peer,
                              yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_user_data ", command_id, " ", peer.id);
    // CHANGES
    std::unordered_map<std::string, AnyElement> new_userdata;
    if (!data_val || !yyjson_is_obj(data_val)) {
        return on_error(command_id, peer.id,
                        ERROR_CANNOT_PARSE_JSON + " Data is missing or not an object");
    }
    yyjson_val *userdata_val = yyjson_obj_get(data_val, "user_data");
    if (!userdata_val || !yyjson_is_obj(userdata_val)) {
        return on_error(command_id, peer.id,
                        ERROR_CANNOT_PARSE_JSON + " Lobby tags missing or not object");
    }
    std::string error = decode_object(userdata_val, new_userdata);
    if (!error.empty()) {
        return on_error(command_id, peer.id, error);
    }
    for (const auto &userdata_pair : new_userdata) {
        if (std::holds_alternative<std::monostate>(userdata_pair.second.value)) {
            peer.user_data.erase(userdata_pair.first);
            continue;
        }
        peer.user_data[userdata_pair.first] = userdata_pair.second;
    }
    // NOTIFICATION
    std::string user_data_string = AnyElement{peer.user_data}.to_string();
    if (peer.lobby_id != "") {
        auto &lobby = game.lobbies[peer.lobby_id];
        std::string notification_others = NOTIFICATION_USER_DATA;
        notification_others.replace(notification_others.find("%s"), 2, user_data_string);
        notification_others.replace(notification_others.find("%s"), 2, peer.id);
        notification_others.replace(notification_others.find("%s"), 2, EMPTY_STRING);
        for (auto lobby_peer_id : game.lobbies[peer.lobby_id].peer_ids) {
            if (lobby_peer_id == peer.id) {
                continue;
            }
            send(lobby_peer_id, notification_others, uWS::OpCode::TEXT);
        }
    }
    std::string notification_self = NOTIFICATION_USER_DATA;
    notification_self.replace(notification_self.find("%s"), 2, user_data_string);
    notification_self.replace(notification_self.find("%s"), 2, peer.id);
    notification_self.replace(notification_self.find("%s"), 2, command_id);
    send(peer.id, notification_self, uWS::OpCode::TEXT);
}

void GameThread::on_lobby_ready(GameData &game, std::string command_id, PeerData &peer,
                                yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_lobby_ready ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    if (peer.ready) {
        return on_error(command_id, peer.id, ERROR_PEER_IS_READY);
    }
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_on_ready") != game.enabled_callbacks.end()) {
        bool has_error = false;
        std::vector<AnyElement> args(1);
        args[0] = AnyElement{true};
        auto func_result =
            scripted_function_call(peer.id, peer.lobby_id, game, "_on_ready", true, args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(command_id, peer.id, std::get<std::string>(func_result.value), false,
                            has_error);
        }
    }
    set_lobby_ready(game.lobbies[peer.lobby_id], peer, command_id, true);
}

void GameThread::on_lobby_unready(GameData &game, std::string command_id, PeerData &peer,
                                  yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_lobby_unready ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    if (!peer.ready) {
        return on_error(command_id, peer.id, ERROR_PEER_IS_NOT_READY);
    }
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_on_ready") != game.enabled_callbacks.end()) {
        bool has_error = false;
        std::vector<AnyElement> args(1);
        args[0] = AnyElement{false};
        auto func_result =
            scripted_function_call(peer.id, peer.lobby_id, game, "_on_ready", true, args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(command_id, peer.id, std::get<std::string>(func_result.value), false,
                            has_error);
        }
    }
    set_lobby_ready(game.lobbies[peer.lobby_id], peer, command_id, false);
}

void GameThread::set_lobby_ready(LobbyData &lobby, PeerData &peer, std::string command_id,
                                 bool ready) {
    // CHANGES
    peer.ready = ready;
    // NOTIFICATION
    std::string notification_others = NOTIFICATION_PEER_READY;
    if (!ready) {
        notification_others = NOTIFICATION_PEER_UNREADY;
    }
    notification_others.replace(notification_others.find("%s"), 2, peer.id);
    notification_others.replace(notification_others.find("%s"), 2, EMPTY_STRING);
    for (auto lobby_peer_id : lobby.peer_ids) {
        if (lobby_peer_id == peer.id && command_id != "") {
            continue;
        }
        send(lobby_peer_id, notification_others, uWS::OpCode::TEXT);
    }
    if (command_id == "") {
        return;
    }
    std::string notification_self = NOTIFICATION_PEER_READY;
    if (!ready) {
        notification_self = NOTIFICATION_PEER_UNREADY;
    }
    notification_self.replace(notification_self.find("%s"), 2, peer.id);
    notification_self.replace(notification_self.find("%s"), 2, command_id);
    send(peer.id, notification_self, uWS::OpCode::TEXT);
}

void GameThread::on_seal_lobby(GameData &game, std::string command_id, PeerData &peer,
                               yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_seal_lobby ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    if (lobby.host != peer.id) {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_HOST);
    }
    if (lobby.sealed) {
        return on_error(command_id, peer.id, ERROR_LOBBY_SEALED);
    }
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_on_seal") != game.enabled_callbacks.end()) {
        bool has_error = false;
        std::vector<AnyElement> args(1);
        args[0] = AnyElement{true};
        auto func_result =
            scripted_function_call(peer.id, peer.lobby_id, game, "_on_seal", true, args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(command_id, peer.id, std::get<std::string>(func_result.value), false,
                            has_error);
        }
    }
    set_lobby_sealed(lobby, peer.id, command_id, true);
}

void GameThread::on_unseal_lobby(GameData &game, std::string command_id, PeerData &peer,
                                 yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_unseal_lobby ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    if (lobby.host != peer.id) {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_HOST);
    }
    if (!lobby.sealed) {
        return on_error(command_id, peer.id, ERROR_LOBBY_NOT_SEALED);
    }
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_on_seal") != game.enabled_callbacks.end()) {
        bool has_error = false;
        std::vector<AnyElement> args(1);
        args[0] = AnyElement{false};
        auto func_result =
            scripted_function_call(peer.id, peer.lobby_id, game, "_on_seal", true, args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(command_id, peer.id, std::get<std::string>(func_result.value), false,
                            has_error);
        }
    }
    set_lobby_sealed(lobby, peer.id, command_id, false);
}

void GameThread::set_lobby_sealed(LobbyData &lobby, std::string peer_id, std::string command_id,
                                  bool sealed) {
    // CHANGES
    lobby.sealed = sealed;
    // NOTIFICATION
    std::string notification_others = NOTIFICATION_LOBBY_UNSEALED;
    if (sealed) {
        notification_others = NOTIFICATION_LOBBY_SEALED;
    }
    notification_others.replace(notification_others.find("%s"), 2, EMPTY_STRING);
    for (auto lobby_peer_id : lobby.peer_ids) {
        if (lobby_peer_id == peer_id && command_id != "") {
            continue;
        }
        send(lobby_peer_id, notification_others, uWS::OpCode::TEXT);
    }
    if (command_id == "") {
        return;
    }
    std::string notification_self = NOTIFICATION_LOBBY_UNSEALED;
    if (sealed) {
        notification_self = NOTIFICATION_LOBBY_SEALED;
    }
    notification_self.replace(notification_self.find("%s"), 2, command_id);
    send(peer_id, notification_self, uWS::OpCode::TEXT);
}

AnyElement GameThread::scripted_function_call(std::string peer_id, std::string &lobby_id, GameData &game,
                                              std::string funcname, bool override,
                                              std::vector<AnyElement> &args, bool &has_error) {
    if (game.lua.enabled) {
        auto result = game.lua.func_call(funcname, args, peer_id, lobby_id, game.id, has_error);
        // if dictionary with error, put error
        auto result_dict = std::get_if<std::unordered_map<std::string, AnyElement>>(&result.value);
        if (result_dict && result_dict->find("error") != result_dict->end()) {
            has_error = true;
            // logical error generates events too
            notify_lobby_changes(game, lobby_id);
            return (*result_dict)["error"];
        }
        notify_lobby_changes(game, lobby_id);
        return result;
    }
    /*
    if (game.angelscript.engine != nullptr) {
        asIScriptContext *ctx = game.angelscript.engine->CreateContext();
        ctx->Prepare(game.angelscript.func);
        ctx->Execute();
        if (!ctx) {
            return AnyElement{"Failed to create AngelScript context"};
        }
        ctx->Release();
    }
    */
    return AnyElement{std::monostate{}};
}

void GameThread::notify_lobby_changes(GameData &game, std::string &lobby_id) {
    auto &lobby = game.lobbies[lobby_id];

    for (const auto &peer_id : lobby.peer_ids) {
        auto &peer = game.peers[peer_id];
        if (peer.private_data_dirty) {
            peer.private_data_dirty = false;
            std::string private_data_notification =
                "{"
                "\"command\": \"data_to\","
                "\"message\": \"Peer private data\","
                "\"data\": {"
                "\"peer_data\": %s,"
                "\"target_peer\": \"%s\","
                "\"is_private\": true"
                "}"
                "}";
            private_data_notification.replace(private_data_notification.find("%s"), 2,
                                              AnyElement{peer.private_data}.to_string());
            private_data_notification.replace(private_data_notification.find("%s"), 2, peer_id);
            // only send to self
            send(peer_id, private_data_notification, uWS::OpCode::TEXT);
        }
        if (peer.public_data_dirty) {
            peer.public_data_dirty = false;
            // Notify peer public data update
            std::string public_data_notification =
                "{"
                "\"command\": \"data_to\","
                "\"message\": \"Peer public data\","
                "\"data\": {"
                "\"peer_data\": %s,"
                "\"target_peer\": \"%s\","
                "\"is_private\": false"
                "}"
                "}";
            public_data_notification.replace(public_data_notification.find("%s"), 2,
                                             AnyElement{peer.public_data}.to_string());
            public_data_notification.replace(public_data_notification.find("%s"), 2, peer_id);
            for (const auto &lobby_peer_id : lobby.peer_ids) {
                send(lobby_peer_id, public_data_notification, uWS::OpCode::TEXT);
            }
        }
    }

    if (lobby.tags_dirty) {
        lobby.tags_dirty = false;
        std::string tags_notification =
            "{"
            "\"command\": \"lobby_tags\","
            "\"message\": \"Tags Set\","
            "\"data\": {"
            "\"tags\": %s,"
            "\"id\": \"%s\""
            "}"
            "}";
        tags_notification.replace(tags_notification.find("%s"), 2,
                                  AnyElement{lobby.tags}.to_string());
        tags_notification.replace(tags_notification.find("%s"), 2, lobby_id);
        for (auto &peer_id : lobby.peer_ids) {
            send(peer_id, tags_notification, uWS::OpCode::TEXT);
        }
    }

    if (lobby.sealed_dirty) {
        lobby.sealed_dirty = false;
        if (lobby.sealed) {
            std::string sealed_notification = NOTIFICATION_LOBBY_SEALED;
            sealed_notification.replace(sealed_notification.find("%s"), 2, EMPTY_STRING);
            for (const auto &peer_id : lobby.peer_ids) {
                send(peer_id, sealed_notification, uWS::OpCode::TEXT);
            }
        } else {
            std::string unsealed_notification = NOTIFICATION_LOBBY_UNSEALED;
            unsealed_notification.replace(unsealed_notification.find("%s"), 2, EMPTY_STRING);
            for (const auto &peer_id : lobby.peer_ids) {
                send(peer_id, unsealed_notification, uWS::OpCode::TEXT);
            }
        }
    }

    if (lobby.public_data_dirty) {
        lobby.public_data_dirty = false;
        // Notify lobby public data update
        std::string public_data_notification =
            "{"
            "\"command\": \"lobby_data\","
            "\"message\": \"Lobby Public Data\","
            "\"data\": {"
            "\"lobby_data\": %s,"
            "\"is_private\": false"
            "}"
            "}";
        public_data_notification.replace(public_data_notification.find("%s"), 2,
                                         AnyElement{lobby.public_data}.to_string());
        for (const auto &peer_id : lobby.peer_ids) {
            send(peer_id, public_data_notification, uWS::OpCode::TEXT);
        }
    }
    // DO NOT SEND PRIVATE DATA FOR SCRIPTED LOBBY
    if (lobby.private_data_dirty && false) {
        lobby.private_data_dirty = false;
        // Notify lobby private data update
        std::string private_data_notification =
            "{"
            "\"command\": \"lobby_data\","
            "\"message\": \"Lobby Private Data\","
            "\"data\": {"
            "\"lobby_data\": %s,"
            "\"is_private\": false,="
            "}"
            "}";
        private_data_notification.replace(private_data_notification.find("%s"), 2,
                                          AnyElement{lobby.private_data}.to_string());
        for (const auto &peer_id : lobby.peer_ids) {
            send(peer_id, private_data_notification, uWS::OpCode::TEXT);
        }
    }
}
