#include "game_thread.h"
#include "INIReader.h"
#include "lua.h"
#include <regex>

std::string EMPTY_STRING = "";

std::string ERROR_CANNOT_PARSE_JSON = "Cannot parse json";
std::string ERROR_PEER_NOT_FOUND = "Peer not found";
std::string ERROR_PEER_ALREADY_EXISTS = "Peer already exists";
std::string ERROR_PEER_NOT_IN_A_LOBBY = "Peer not in a lobby";
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

std::string NOTIFICATION_ERROR = "{"
    "\"command\": \"error\","
    "\"message\": \"%s\","
    "\"data\": {"
        "\"peer_id\": \"%s\","
        "\"id\": \"%s\""
    "}"
"}";
std::string NOTIFICATION_LOBBY_CREATED = "{"
    "\"command\": \"lobby_created\","
    "\"message\": \"Lobby created\","
    "\"data\": {"
        "\"lobby\": %s,"
        "\"peers\": %s,"
        "\"id\": \"%s\""
    "}"
"}";
std::string NOTIFICATION_LOBBY_UNSEALED = "{"
    "\"command\": \"lobby_unsealed\","
    "\"message\": \"Lobby unsealed\","
    "\"data\": {"
        "\"peer_id\": \"%s\","
        "\"id\": \"%s\""
    "}"
"}";
std::string NOTIFICATION_LOBBY_SEALED = "{"
    "\"command\": \"lobby_sealed\","
    "\"message\": \"Lobby sealed\","
    "\"data\": {"
        "\"id\": \"%s\""
    "}"
"}";
std::string NOTIFICATION_PEER_READY = "{"
    "\"command\": \"peer_ready\","
    "\"message\": \"Peer ready\","
    "\"data\": {"
        "\"peer_id\": \"%s\","
        "\"id\": \"%s\""
    "}"
"}";
std::string NOTIFICATION_PEER_UNREADY = "{"
    "\"command\": \"peer_unready\","
    "\"message\": \"Peer unready\","
    "\"data\": {"
        "\"peer_id\": \"%s\","
        "\"id\": \"%s\""
    "}"
"}";
std::string NOTIFICATION_LOBBY_LEFT = "{"
    "\"command\": \"lobby_left\","
    "\"message\": \"Host Left\","
    "\"data\": {"
        "\"id\": \"%s\""
    "}"
"}";
std::string NOTIFICATION_PEER_LEFT = "{"
    "\"command\": \"peer_left\","
    "\"message\": \"Peer left\","
    "\"data\": {"
        "\"peer_id\": \"%s\","
        "\"id\": \"%s\""
    "}"
"}";
std::string NOTIFICATION_PEER_KICKED = "{"
    "\"command\": \"peer_left\","
    "\"message\": \"Peer kicked\","
    "\"data\": {"
        "\"peer_id\": \"%s\","
        "\"kicked\": true,"
        "\"id\": \"%s\""
    "}"
"}";
std::string NOTIFICATION_LOBBY_KICKED = "{"
    "\"command\": \"lobby_kicked\","
    "\"message\": \"Lobby kicked\""
"}";
std::string NOTIFICATION_PEER_STATE = "{"
    "\"command\": \"peer_state\","
    "\"message\": \"Initial Message\","
    "\"data\": {"
        "\"peer\": %s"
    "}"
"}";
std::string NOTIFICATION_LOBBY_CALL = "{"
    "\"command\": \"lobby_call\","
    "\"message\": \"Lobby Call\","
    "\"data\": {"
        "\"result\": %s,"
        "\"id\": \"%s\""
    "}"
"}";
std::string NOTIFICATION_USER_DATA = "{"
    "\"command\": \"user_data\","
    "\"message\": \"User Data\","
    "\"data\": {"
        "\"user_data\": %s,"
        "\"id\": \"%s\""
    "}"
"}";
std::string NOTIFICATION_TAGS = "{"
    "\"command\": \"lobby_tags\","
    "\"message\": \"Tags Set\","
    "\"data\": {"
        "\"tags\": %s,"
        "\"id\": \"%s\""
    "}"
"}";
std::string NOTIFICATION_CHAT = "{"
    "\"command\": \"chat\","
    "\"message\": \"Chat\","
    "\"data\": {"
        "\"peer_id\": \"%s\","
        "\"message\": \"%s\","
        "\"id\": \"%s\""
    "}"
"}";
std::string NOTIFICATION_PEER_RECONNECTED = "{"
    "\"command\": \"peer_reconnected\","
    "\"message\": \"Peer reconnected\","
    "\"data\": {"
        "\"peer_id\": \"%s\""
    "}"
"}";
std::string NOTIFICATION_LOBBY_JOINED = "{"
    "\"command\": \"joined_lobby\","
    "\"message\": \"Lobby joined\","
    "\"data\": {"
        "\"lobby\": %s,"
        "\"peers\": %s,"
        "\"id\": \"%s\""
    "}"
"}";
std::string NOTIFICATION_PEER_JOINED = "{"
    "\"command\": \"peer_joined\","
    "\"message\": \"Peer joined\","
    "\"data\": {"
        "\"peer\": %s"
    "}"
"}";
std::string NOTIFICATION_PEER_DICONNECTED = "{"
    "\"command\": \"peer_disconnected\","
    "\"message\": \"Peer disconnected\","
    "\"data\": {"
        "\"peer_id\": \"%s\""
    "}"
"}";

GameThread::GameThread(bool verbose,
    std::string log_folder,
    std::string scripts_folder,
    moodycamel::BlockingReaderWriterQueue<WebSocketMessage> &message_queue,
    uWS::Loop *loop,
    WebSocketServer<true> *webserver,
    WebSocketServer<false> *webserver_no_ssl): logger(verbose, log_folder + "/game.txt"),
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
    for (const auto& entry : std::filesystem::directory_iterator(scripts_folder)) {
        if (entry.is_directory()) {
            std::string folder_name = entry.path().filename().string();
            logger.debug_log("[GameThread] on_load_game ", folder_name);
            INIReader config_reader(scripts_folder +"/" + folder_name + "/config.ini");
            if (config_reader.ParseError() < 0) {
                logger.debug_log("[GameThread] Cannot open " + scripts_folder + "/" + folder_name + "/config.ini");
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
            games.emplace(folder_name, GameData{
                .id = folder_name,
                .entrypoint = script_entrypoint,
                .tick_rate = tickrate,
                .enabled_callbacks = enabled_callbacks,
                .lua = ScriptLua{
                    .script_language = script_language,
                    .scripts_folder = scripts_folder,
                    .folder_name = folder_name,
                    .script_entrypoint = script_entrypoint,
                    .logs_folder = logs_folder,
                    .game_thread = this,
                },
                //.angelscript = ScriptAngelScript(script_language, scripts_folder, folder_name, script_entrypoint, logger, config_reader)
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
    while (true) {
        handle_events();
        handle_disconnects();
    }
    unload_games();
}

void GameThread::handle_events() {
    WebSocketMessage message;
    if (!message_queue.try_dequeue(message)) {
        // no new events
        return;
    }
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
                on_error(EMPTY_STRING, message.id, ERROR_CANNOT_PARSE_JSON + " Initial decode", true);
                break;
            }

            yyjson_val *root = yyjson_doc_get_root(doc);
            if (!root || !yyjson_is_obj(root)) {
                yyjson_doc_free(doc);
                on_error(EMPTY_STRING, message.id, ERROR_CANNOT_PARSE_JSON + " Root is not object", true);
                break;
            }
            // get command
            yyjson_val *command_val = yyjson_obj_get(root, "command");
            if (!command_val || !yyjson_is_str(command_val)) {
                yyjson_doc_free(doc);
                on_error(EMPTY_STRING, message.id, ERROR_CANNOT_PARSE_JSON + " Command missing or not string", true);
                break;
            }
            
            std::string command = yyjson_get_str(command_val);

            // --- Extract data ---
            yyjson_val *data_val = yyjson_obj_get(root, "data");

            // Extract command id if possible
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
            } else if (command == "list_lobbys") {
                on_list_lobbys(game, std::string(command_id), peer, data_val);
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
                on_error(std::string(command_id), message.id, ERROR_UNKOWN_COMMAND + " " + std::string(command), true);
            }
            yyjson_doc_free(doc);
            break;
    }
}

void GameThread::handle_disconnects() {
    for (auto &game : games) {
        auto &game_data = game.second;
        for (auto &peer : game_data.disconnected_peers) {
            if (get_time_now() - peer.second > MAX_RECONNECTION_TIME) {
                remove_peer_from_lobby(game_data, peer.first);
            }
        }
    }
}

void GameThread::remove_peer_from_lobby(GameData &game, std::string peer_id) {
    auto &peer = game.peers[peer_id];
    if (peer.lobby_id != "") {
        auto &lobby = game.lobbies[peer.lobby_id];
        lobby.peer_ids.erase(peer_id);
    }
    game.peers.erase(peer_id);
}

void GameThread::send(std::string &peer_id, const std::string &message, uWS::OpCode opCode) {
    if (webserver_ssl != nullptr) {
        loop->defer([id = peer_id, msg = message, webserver = webserver_ssl]() {
            webserver->send(id, msg, uWS::OpCode::TEXT);
        });
    } else {
        loop->defer([id = peer_id, msg = message, webserver = webserver_nossl](){
            webserver->send(id, msg, uWS::OpCode::TEXT);
        });
    }
}

void GameThread::on_connect(GameData &game, std::string &peer_id, std::string &game_id, std::string &reconnection_token) {
    logger.debug_log("[GameThread] on_connect ", peer_id, " ", game_id);
    if (game.peers.find(peer_id) != game.peers.end()) {
        auto &peer = game.peers[peer_id];
        peer.reconnection_token = reconnection_token;
        game.disconnected_peers.erase(peer_id);
    } else {
        game.peers.emplace(peer_id, PeerData {
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
    auto &lobby = game.lobbies[peer.lobby_id];
    for (auto lobby_peer_id : lobby.peer_ids) {
        if (lobby_peer_id == peer_id) {
            continue;
        }
        send(lobby_peer_id, notification, uWS::OpCode::TEXT);
    }
}

void GameThread::on_error(std::string command_id, std::string &peer_id, std::string message, bool close) {
    if (close) {
        logger.error_log("[GameThread] on_error ", command_id, " ", peer_id, " ", message);
    }
    std::string result = NOTIFICATION_ERROR;
    result.replace(result.find("%s"), 2, message);
    result.replace(result.find("%s"), 2, peer_id);
    result.replace(result.find("%s"), 2, command_id);
    if (close) {
        send(peer_id, message, uWS::OpCode::CLOSE);
    } else {
        send(peer_id, result, uWS::OpCode::TEXT);
    }
}

void GameThread::on_lobby_call(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_lobby_call ", command_id, " ", peer.id);
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto func_name = decode_string_or_default(data_val, "function", "");
    if (func_name == "") {
        return on_error(command_id, peer.id, ERROR_INVALID_FUNCTION);
    }
    AnyElement args{
        std::vector<AnyElement>()
    };
    yyjson_val *inputs_val = yyjson_obj_get(data_val, "inputs");
    if (inputs_val && yyjson_is_arr(inputs_val)) {
        decode_array(inputs_val, args);
    }
    if (!std::holds_alternative<std::vector<AnyElement>>(args.value)) {
        return on_error(command_id, peer.id, ERROR_INVALID_ARGUMENTS);
    }
    auto& array_value = std::get<std::vector<AnyElement>>(args.value);
    array_value.insert(array_value.begin(), AnyElement{peer.id});
    // SCRIPTED CALL
    bool has_error = false;
    auto func_result = scripted_function_call(peer.lobby_id, game, func_name, true, array_value, has_error);
    if (has_error && std::holds_alternative<std::string>(func_result.value)) {
        return on_error(command_id, peer.id, std::get<std::string>(func_result.value));
    } else {
        std::string notification = NOTIFICATION_LOBBY_CALL;
        notification.replace(notification.find("%s"), 2, func_result.to_string());
        notification.replace(notification.find("%s"), 2, command_id);
        send(peer.id, notification, uWS::OpCode::TEXT);
    }
}

void GameThread::on_quick_join(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_quick_join ", command_id, " ", peer.id);
    if (peer.lobby_id != "") {
        return on_error(command_id, peer.id, ERROR_PEER_IS_IN_A_LOBBY);
    }
}

void GameThread::on_create_lobby(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val) {
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
    game.lobbies.emplace(small_uuid, LobbyData{
        .id = small_uuid,
        .create_time = get_time_now(),
        .host = peer.id,
        .game_id = peer.game_id,
        .max_players = decode_int_or_default(data_val, "max_players", 0),
        .password = decode_string_or_default(data_val, "password", ""),
        .tags = lobby_tags,
        .name = decode_string_or_default(data_val, "name", ""),
        .peer_ids = {peer.id},
    });
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_on_create") != game.enabled_callbacks.end()) {
        bool has_error = false;
        std::vector<AnyElement> args(1);
        args[0] = AnyElement{peer.id};
        auto func_result = scripted_function_call(peer.lobby_id, game, "_on_create", true, args, has_error);
        if (has_error && std::holds_alternative<std::string>(func_result.value)) {
            // revert the changes
            peer.leave_lobby();
            game.lobbies.erase(small_uuid);
            return on_error(command_id, peer.id, std::get<std::string>(func_result.value));
        }
    }
    // NOTIFICATION
    std::string notification = NOTIFICATION_LOBBY_CREATED;
    notification.replace(notification.find("%s"), 2, game.lobbies[small_uuid].to_string());
    notification.replace(notification.find("%s"), 2, game.peers_to_string(small_uuid));
    notification.replace(notification.find("%s"), 2, command_id);
    send(peer.id, notification, uWS::OpCode::TEXT);
}

void GameThread::on_join_lobby(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_join_lobby ", command_id, " ", peer.id);
    // PRECONDITIONS
    bool reconnecting = false;
    std::string lobby_id = decode_string_or_default(data_val, "lobby_id", "");
    if (peer.disconnected && peer.lobby_id != "") {
        reconnecting = true;
        lobby_id = peer.lobby_id;
    }
    if (lobby_id == "") {
        return on_error(command_id, peer.id, "Invalid lobby id");
    }
    auto &lobby = game.lobbies[lobby_id];
    if (!reconnecting) {
        if (peer.lobby_id != "") {
            return on_error(command_id, peer.id, ERROR_PEER_IS_IN_A_LOBBY);
        }
        if (game.lobbies.find(lobby_id) == game.lobbies.end()) {
            return on_error(command_id, peer.id, ERROR_LOBBY_NOT_FOUND + " " + lobby_id);
        }
        if (lobby.password != decode_string_or_default(data_val, "password", "")) {
            return on_error(command_id, peer.id, "Invalid password");
        }
        if (lobby.max_players != 0 && lobby.peer_ids.size() >= lobby.max_players) {
            return on_error(command_id, peer.id, "Lobby is full");
        }
    }
    // SCRIPTED CALL
    if (!reconnecting) {
        if (game.enabled_callbacks.find("_on_join") != game.enabled_callbacks.end()) {
            bool has_error = false;
            std::vector<AnyElement> args(1);
            args[0] = AnyElement{peer.id};
            auto func_result = scripted_function_call(lobby_id, game, "_on_join", true, args, has_error);
            if (has_error && std::holds_alternative<std::string>(func_result.value)) {
                return on_error(command_id, peer.id, std::get<std::string>(func_result.value));
            }
        }
    }
    // CHANGES
    if (reconnecting) {
        peer.ready = false;
    } else {
        peer.order_id = lobby.order_id_counter++;
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
}

void GameThread::on_leave_lobby(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_leave_lobby ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    // SCRIPTED CALL
    // CHANGES
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
        game.lobbies.erase(peer.lobby_id);
    } else {
        lobby.peer_ids.erase(peer.id);
        peer.leave_lobby();
        std::string notification_others = NOTIFICATION_PEER_LEFT;
        notification_others.replace(notification_others.find("%s"), 2, peer.id);
        notification_others.replace(notification_others.find("%s"), 2, EMPTY_STRING);
        for (auto lobby_peer_id : lobby.peer_ids) {
            if (lobby_peer_id == peer.id) {
                continue;
            }
            send(lobby_peer_id, notification_others, uWS::OpCode::TEXT);
        }
        std::string notification_self = NOTIFICATION_LOBBY_LEFT;
        notification_self.replace(notification_self.find("%s"), 2, command_id);
        send(peer.id, notification_self, uWS::OpCode::TEXT);
    }
}

void GameThread::on_list_lobbys(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_list_lobbys ", command_id, " ", peer.id);
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
}

std::string strip_BBCode(const std::string& input) {
    static const std::regex bbcode_regex(R"(\[.*?\])");
    return std::regex_replace(input, bbcode_regex, "");
}

void GameThread::on_chat_lobby(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_chat_lobby ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    std::string message = decode_string_or_default(data_val, "message", "");
    message = strip_BBCode(message);
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_on_tags") != game.enabled_callbacks.end()) {
        bool has_error = false;
        std::vector<AnyElement> args(1);
        args[0] = AnyElement{message};
        auto func_result = scripted_function_call(peer.lobby_id, game, "_on_chat", true, args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(command_id, peer.id, std::get<std::string>(func_result.value));
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
    notification_others.replace(notification_others.find("%s"), 2, peer.id);
    notification_others.replace(notification_others.find("%s"), 2, message);
    notification_self.replace(notification_self.find("%s"), 2, command_id);
    send(peer.id, notification_self, uWS::OpCode::TEXT);
}

void GameThread::on_lobby_tags(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val) {
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
        return on_error(command_id, peer.id, ERROR_CANNOT_PARSE_JSON + " Data is missing or not an object");
    }
    yyjson_val *tags_val = yyjson_obj_get(data_val, "tags");
    if (!tags_val || !yyjson_is_obj(tags_val)) {
        return on_error(command_id, peer.id, ERROR_CANNOT_PARSE_JSON + " Lobby tags missing or not object");
    }
    std::string error = decode_object(tags_val, new_tags);
    if (!error.empty()) {
        return on_error(command_id, peer.id, error);
    }
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_on_tags") != game.enabled_callbacks.end()) {
        bool has_error = false;
        std::vector<AnyElement> args(2);
        args[0] = AnyElement{new_tags};
        args[1] = AnyElement{true};
        auto func_result = scripted_function_call(peer.lobby_id, game, "_on_tags", true, args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(command_id, peer.id, std::get<std::string>(func_result.value));
        }
    }
    // CHANGES
    lobby.tags = new_tags;
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

void GameThread::on_kick_peer(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_kick_peer ", command_id, " ", peer.id);
    // PRECONDITIONS
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    if (lobby.host != peer.id) {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_HOST);
    }
    auto &peer_ids = game.lobbies[peer.lobby_id].peer_ids;
    if (peer_ids.find(peer.id) == peer_ids.end()) {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    std::string kicked_peer_id = decode_string_or_default(data_val, "peer_id", "");
    if (kicked_peer_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    // SCRIPTED CALL
    if (game.enabled_callbacks.find("_on_kick") != game.enabled_callbacks.end()) {
        bool has_error = false;
        std::vector<AnyElement> args(2);
        args[0] = AnyElement{kicked_peer_id};
        args[1] = AnyElement{true};
        auto func_result = scripted_function_call(peer.lobby_id, game, "_on_kick", true, args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(command_id, peer.id, std::get<std::string>(func_result.value));
        }
    }
    // CHANGES
    lobby.peer_ids.erase(kicked_peer_id);
    peer.leave_lobby();
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

void GameThread::on_user_data(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val) {
    logger.debug_log("[GameThread] on_user_data ", command_id, " ", peer.id);
    // CHANGES
    auto decode_message = decode_object(data_val, peer.user_data);
    if (decode_message != EMPTY_STRING) {
        return on_error(command_id, peer.id, ERROR_CANNOT_PARSE_JSON + decode_message);
    }
    // NOTIFICATION
    std::string user_data_string = AnyElement{peer.user_data}.to_string();
    if (peer.lobby_id != "") {
        auto &lobby = game.lobbies[peer.lobby_id];
        std::string notification_others = NOTIFICATION_USER_DATA;
        notification_others.replace(notification_others.find("%s"), 2, user_data_string);
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
    notification_self.replace(notification_self.find("%s"), 2, command_id);
    send(peer.id, notification_self, uWS::OpCode::TEXT);
}

void GameThread::on_lobby_ready(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val) {
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
        std::vector<AnyElement> args(2);
        args[0] = AnyElement{peer.id};
        args[1] = AnyElement{true};
        auto func_result = scripted_function_call(peer.lobby_id, game, "_on_ready", true, args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(command_id, peer.id, std::get<std::string>(func_result.value));
        }
    }
    // CHANGES
    peer.ready = true;
    // NOTIFICATION
    std::string notification_others = NOTIFICATION_PEER_READY;
    notification_others.replace(notification_others.find("%s"), 2, peer.id);
    notification_others.replace(notification_others.find("%s"), 2, EMPTY_STRING);
    for (auto lobby_peer_id : game.lobbies[peer.lobby_id].peer_ids) {
        if (lobby_peer_id == peer.id) {
            continue;
        }
        send(lobby_peer_id, notification_others, uWS::OpCode::TEXT);
    }
    std::string notification_self = NOTIFICATION_PEER_READY;
    notification_self.replace(notification_self.find("%s"), 2, peer.id);
    notification_self.replace(notification_self.find("%s"), 2, command_id);
    send(peer.id, notification_self, uWS::OpCode::TEXT);
}

void GameThread::on_lobby_unready(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val) {
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
        std::vector<AnyElement> args(2);
        args[0] = AnyElement{peer.id};
        args[1] = AnyElement{false};
        auto func_result = scripted_function_call(peer.lobby_id, game, "_on_ready", true, args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(command_id, peer.id, std::get<std::string>(func_result.value));
        }
    }
    // CHANGES
    peer.ready = false;
    // NOTIFICATION
    std::string notification_others = NOTIFICATION_PEER_UNREADY;
    notification_others.replace(notification_others.find("%s"), 2, peer.id);
    notification_others.replace(notification_others.find("%s"), 2, EMPTY_STRING);
    for (auto lobby_peer_id : game.lobbies[peer.lobby_id].peer_ids) {
        if (lobby_peer_id == peer.id) {
            continue;
        }
        send(lobby_peer_id, notification_others, uWS::OpCode::TEXT);
    }
    std::string notification_self = NOTIFICATION_PEER_UNREADY;
    notification_self.replace(notification_self.find("%s"), 2, peer.id);
    notification_self.replace(notification_self.find("%s"), 2, command_id);
    send(peer.id, notification_self, uWS::OpCode::TEXT);
}

void GameThread::on_seal_lobby(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val) {
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
        std::vector<AnyElement> args(2);
        args[0] = AnyElement{peer.id};
        args[1] = AnyElement{true};
        auto func_result = scripted_function_call(peer.lobby_id, game, "_on_seal", true, args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(command_id, peer.id, std::get<std::string>(func_result.value));
        }
    }
    // CHANGES
    lobby.sealed = true;
    // NOTIFICATION
    std::string notification_others = NOTIFICATION_LOBBY_SEALED;
    notification_others.replace(notification_others.find("%s"), 2, EMPTY_STRING);
    for (auto lobby_peer_id : game.lobbies[peer.lobby_id].peer_ids) {
        if (lobby_peer_id == peer.id) {
            continue;
        }
        send(lobby_peer_id, notification_others, uWS::OpCode::TEXT);
    }
    std::string notification_self = NOTIFICATION_LOBBY_SEALED;
    notification_self.replace(notification_self.find("%s"), 2, command_id);
    send(peer.id, notification_self, uWS::OpCode::TEXT);
}

void GameThread::on_unseal_lobby(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val) {
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
        std::vector<AnyElement> args(2);
        args[0] = AnyElement{peer.id};
        args[1] = AnyElement{false};
        auto func_result = scripted_function_call(peer.lobby_id, game, "_on_seal", true, args, has_error);
        if (std::holds_alternative<std::string>(func_result.value)) {
            return on_error(command_id, peer.id, std::get<std::string>(func_result.value));
        }
    }
    // CHANGES
    lobby.sealed = false;
    // NOTIFICATION
    std::string notification_others = NOTIFICATION_LOBBY_UNSEALED;
    notification_others.replace(notification_others.find("%s"), 2, EMPTY_STRING);
    for (auto lobby_peer_id : game.lobbies[peer.lobby_id].peer_ids) {
        if (lobby_peer_id == peer.id) {
            continue;
        }
        send(lobby_peer_id, notification_others, uWS::OpCode::TEXT);
    }
    std::string notification_self = NOTIFICATION_LOBBY_UNSEALED;
    notification_self.replace(notification_self.find("%s"), 2, command_id);
    send(peer.id, notification_self, uWS::OpCode::TEXT);
}

AnyElement GameThread::scripted_function_call(std::string &lobby_id, GameData &game, std::string funcname, bool override, std::vector<AnyElement> &args, bool &has_error) {
    if (game.lua.enabled) {
        auto result = game.lua.func_call(funcname, args, lobby_id, game.id, has_error);
        // if dictionary with error, put error
        auto result_dict = std::get_if<std::unordered_map<std::string, AnyElement>>(&result.value);
        if (result_dict && result_dict->find("error") != result_dict->end()) {
            has_error = true;
            return (*result_dict)["error"];
        }
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

std::string GameThread::decode_string_or_default(yyjson_val *object, std::string key, std::string default_value) {
    if (!object || !yyjson_is_obj(object)) {
        return default_value;
    }

    yyjson_val *value = yyjson_obj_get(object, key.c_str());
    if (!value || !yyjson_is_str(value)) {
        return default_value;
    }

    return std::string(yyjson_get_str(value));
}

int GameThread::decode_int_or_default(yyjson_val *object, std::string key, int default_value) {
    if (!object || yyjson_get_type(object) != YYJSON_TYPE_OBJ) {
        return default_value;
    }

    yyjson_val *val = yyjson_obj_get(object, key.c_str());
    if (!val) return default_value;

    if (yyjson_is_int(val)) return static_cast<int>(yyjson_get_int(val));
    if (yyjson_is_uint(val)) return static_cast<int>(yyjson_get_uint(val));
    if (yyjson_is_real(val)) return static_cast<int>(yyjson_get_real(val));

    return default_value;
}

std::string GameThread::decode_array(yyjson_val *array, AnyElement &element) {
    if (!array || yyjson_get_type(array) != YYJSON_TYPE_ARR) {
        return "Not an array";
    }

    std::vector<AnyElement> result;
    yyjson_val *item;
    size_t idx, max;
    max = yyjson_arr_size(array);
    yyjson_arr_foreach(array, idx, max, item) {
        AnyElement child;
        std::string error = decode_value(item, child);
        if (!error.empty()) return error;
        result.push_back(child);
    }

    element.value = result;
    return EMPTY_STRING;
}

std::string GameThread::decode_value(yyjson_val *value, AnyElement &element) {
    if (!value){
        return "Null value";
    }

    switch (yyjson_get_type(value)) {
        case YYJSON_TYPE_NULL:
            element.value = std::monostate{};
            break;
        case YYJSON_TYPE_BOOL:
            element.value = yyjson_get_bool(value);
            break;
        case YYJSON_TYPE_NUM:
            if (yyjson_is_int(value)){
                element.value = yyjson_get_int(value);
            }
            else if (yyjson_is_uint(value)) {
                element.value = static_cast<int64_t>(yyjson_get_uint(value));
            }
            else {
                element.value = yyjson_get_real(value);
            }
            break;
        case YYJSON_TYPE_STR:
            element.value = std::string(yyjson_get_str(value));
            break;
        case YYJSON_TYPE_ARR: {
            std::string error = decode_array(value, element);
            if (!error.empty()) return error;
            break;
        }
        case YYJSON_TYPE_OBJ: {
            std::unordered_map<std::string, AnyElement> map_result;
            std::string error = decode_object(value, map_result);
            if (!error.empty()) return error;
            element.value = map_result;
            break;
        }
        default:
            return "Unknown JSON type";
    }

    return EMPTY_STRING;
}
std::string GameThread::decode_object(yyjson_val *object, std::unordered_map<std::string, AnyElement> &dict) {
    if (!object || yyjson_get_type(object) != YYJSON_TYPE_OBJ) {
        return EMPTY_STRING;
    }

    yyjson_val *key, *val;
    yyjson_obj_iter iter = yyjson_obj_iter_with(object);
    while ((key = yyjson_obj_iter_next(&iter))) {
        val = yyjson_obj_iter_get_val(key);
        if (!yyjson_is_str(key)) {
            return "Non-string key in object";
        }

        AnyElement element;
        std::string error = decode_value(val, element);
        if (!error.empty()) return error;

        dict[yyjson_get_str(key)] = element;
    }

    return EMPTY_STRING;
}
