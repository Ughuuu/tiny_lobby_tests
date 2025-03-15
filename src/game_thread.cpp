#include "game_thread.h"
#include "INIReader.h"

std::string EMPTY_STRING = "";

std::string ERROR_CANNOT_PARSE_JSON = "Cannot parse json";
std::string ERROR_PEER_NOT_FOUND = "Peer not found";
std::string ERROR_PEER_ALREADY_EXISTS = "Peer already exists";
std::string ERROR_PEER_NOT_IN_A_LOBBY = "Peer not in a lobby";
std::string ERROR_PEER_IS_IN_A_LOBBY = "Peer is in a lobby";
std::string ERROR_PEER_NOT_HOST = "Peer is not the host";
std::string ERROR_PEER_IS_READY = "Peer is ready";
std::string ERROR_PEER_IS_NOT_READY = "Peer is not ready";
std::string ERROR_LOBBY_NOT_SEALED = "Lobby is not sealed";
std::string ERROR_LOBBY_SEALED = "Lobby is sealed";
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

GameThread::GameThread(bool verbose,
    std::string scripts_folder,
    moodycamel::BlockingReaderWriterQueue<WebSocketMessage> &message_queue,
    uWS::Loop *loop,
    WebSocketServer<true> *webserver,
    WebSocketServer<false> *webserver_no_ssl): logger(verbose, "game_thread_log.txt"),
        scripts_folder(scripts_folder),
        message_queue(message_queue),
        loop(loop),
        webserver_ssl(webserver),
        webserver_nossl(webserver_no_ssl) {
    logger.debug_log("[GameThread] on_start");
}

void GameThread::load_games() {
    logger.debug_log("[GameThread] on_load_games");
    for (const auto& entry : std::filesystem::directory_iterator(scripts_folder)) {
        if (entry.is_directory()) {
            std::string folder_name = entry.path().filename().string();
            INIReader config_reader(scripts_folder +"/" + folder_name + "/config.ini");
            if (config_reader.ParseError() < 0) {
                std::cout << "Cannot open " + scripts_folder + "/" + folder_name + "/config.ini" << std::endl;
            }
            std::string script_language = config_reader.Get("game", "language", "lua");
            if (script_language != "lua" && script_language != "angelscript") {
                std::cout << "Unknown script language: " << script_language << std::endl;
                continue;
            }
            std::string script_entrypoint = config_reader.Get("game", "entrypoint", "");
            if (script_language == "lua" && script_entrypoint == "") {
                script_entrypoint = "main.lua";
            }
            if (script_language == "angelscript" && script_entrypoint == "") {
                script_entrypoint = "main.as";
            }
            long tickrate = config_reader.GetInteger("game", "tickrate", -1);
            lua_State *L = nullptr;
            if (script_language == "lua" && std::filesystem::exists(scripts_folder + "/" + folder_name + "/" + script_entrypoint)) {
                L = luaL_newstate();
                if (!L) {
                    std::cerr << "Failed to create Lua state" << std::endl;
                    return;
                }
                luaL_openlibs(L);
                luaL_dostring(L, ("package.path = \"" + scripts_folder + "/" + folder_name + "/?.lua;\" .. package.path").c_str());
                luaL_dofile(L, (scripts_folder + "/" + folder_name + "/" + script_entrypoint).c_str());
            }
            asIScriptEngine *engine;
            asIScriptModule *mod;
            asIScriptFunction *func;
            if (script_language == "angelscript" && std::filesystem::exists(scripts_folder + "/" + folder_name + "/" + script_entrypoint)) {
                engine = asCreateScriptEngine();
                if (!engine) {
                    std::cerr << "Failed to create AngelScript engine" << std::endl;
                    return;
                }
                mod = engine->GetModule("default", asGM_ALWAYS_CREATE);
                if (!mod) {
                    std::cerr << "Failed to create AngelScript module" << std::endl;
                    return;
                }
                const char *script = 
                "void main() {"
                "   print('Hello from AngelScript!');"
                "}";
                mod->AddScriptSection("my_script", script, strlen(script), 0);
                mod->Build();
                func = mod->GetFunctionByName("main");
            }




            games.emplace(folder_name, GameData{
                .id = folder_name,
                .tick_rate = tickrate,
                .entrypoint = script_entrypoint,
                .lua = {L},
                .angelscript = {engine, mod}
            });
        }
    }
}

void GameThread::unload_games() {
    logger.debug_log("[GameThread] on_unload_games");
    for (auto &game : games) {
        if (game.second.lua.L != nullptr) {
            lua_close(game.second.lua.L);
        }
        if (game.second.angelscript.engine != nullptr) {
            game.second.angelscript.engine->Release();
        }
    }
    games.clear();
}

void GameThread::run() {
    load_games();
    // max 10kb
    simdjson::ondemand::parser parser(10000);
    logger.debug_log("[GameThread] on_start_thread");
    simdjson::ondemand::document doc;
    while (true) {
        WebSocketMessage message;
        message_queue.wait_dequeue(message);
        if (games.find(message.game_id) == games.end()) {
            on_error(EMPTY_STRING, message.id, ERROR_GAME_NOT_FOUND, true);
            continue;
        }
        auto &game = games[message.game_id];
        switch (message.event) {
            case WebSocketEvent::OPEN:
                on_connect(game, message.id, message.game_id, message.reconnection_id);
                break;
            case WebSocketEvent::CLOSE:
                on_close(game, message.id);
                break;
            case WebSocketEvent::MESSAGE:
                // decode message
                simdjson::padded_string padded_string(message.message);
                auto parser_error = parser.iterate(padded_string).get(doc);
                if(parser_error != simdjson::SUCCESS) {
                    on_error(EMPTY_STRING, message.id,  ERROR_CANNOT_PARSE_JSON + " Initial decode " + simdjson::error_message(parser_error), true);
                    break;
                }
                simdjson::ondemand::object doc_object = doc.get_object();
                // get command
                std::string_view command;
                auto value_error = doc["command"].get_string().get(command);
                if (value_error != simdjson::SUCCESS) {
                    on_error(EMPTY_STRING, message.id,  ERROR_CANNOT_PARSE_JSON + " Command decode " + simdjson::error_message(value_error), true);
                    break;
                }
                static simdjson::ondemand::object data;
                value_error = doc["data"].get_object().get(data);
                if (value_error != simdjson::SUCCESS) {
                    on_error(EMPTY_STRING, message.id,  ERROR_CANNOT_PARSE_JSON + " Data decode " + simdjson::error_message(value_error));
                    break;
                }
                static std::string_view command_id;
                value_error = data["id"].get_string().get(command_id);
                if (value_error != simdjson::SUCCESS) { /* noop */ }
                if (game.peers.find(message.id) == game.peers.end()) {
                    return on_error(std::string(command_id), message.id, ERROR_PEER_NOT_FOUND, true);
                }
                auto &peer = game.peers[message.id];
                if (command == "lobby_call") {
                    on_lobby_call(game, std::string(command_id), peer, data);
                } else if (command == "quick_join") {
                    on_quick_join(game, std::string(command_id), peer, data);
                } else if (command == "create_lobby") {
                    on_create_lobby(game, std::string(command_id), peer, data);
                } else if (command == "join_lobby") {
                    on_join_lobby(game, std::string(command_id), peer, data);
                } else if (command == "leave_lobby") {
                    on_leave_lobby(game, std::string(command_id), peer, data);
                } else if (command == "list_lobbys") {
                    on_list_lobbys(game, std::string(command_id), peer, data);
                } else if (command == "chat_lobby") {
                    on_chat_lobby(game, std::string(command_id), peer, data);
                } else if (command == "lobby_tags") {
                    on_lobby_tags(game, std::string(command_id), peer, data);
                } else if (command == "kick_peer") {
                    on_kick_peer(game, std::string(command_id), peer, data);
                } else if (command == "user_data") {
                    on_user_data(game, std::string(command_id), peer, data);
                } else if (command == "lobby_ready") {
                    on_lobby_ready(game, std::string(command_id), peer, data);
                } else if (command == "lobby_unready") {
                    on_lobby_unready(game, std::string(command_id), peer, data);
                } else if (command == "seal_lobby") {
                    on_seal_lobby(game, std::string(command_id), peer, data);
                } else if (command == "unseal_lobby") {
                    on_unseal_lobby(game, std::string(command_id), peer, data);
                } else {
                    on_error(std::string(command_id), message.id, ERROR_UNKOWN_COMMAND + " " + std::string(command), true);
                }
                break;
        }
    }
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

void GameThread::on_connect(GameData &game, std::string &peer_id, std::string &game_id, std::string &reconnection_id) {
    logger.debug_log("[GameThread] on_connect ", peer_id, " ", game_id, " ", reconnection_id);
    if (game.peers.find(peer_id) != game.peers.end()) {
        return on_error("", peer_id, ERROR_PEER_ALREADY_EXISTS, true);
    }
    if (reconnection_id != "") {
        if (game.reconnection_to_peer_ids.find(reconnection_id) == game.reconnection_to_peer_ids.end()) {
            // we didn't find any reconnection_id, generate a new one
            reconnection_id = std::to_string(std::rand());
        }
    }
    game.peers.emplace(peer_id, PeerData{
        .id = peer_id,
        .game_id = game_id,
        .reconnection_id = reconnection_id
    });
}

void GameThread::on_close(GameData &game, std::string &peer_id) {
    logger.debug_log("[GameThread] on_close ", peer_id);
    game.peers.erase(peer_id);
}

void GameThread::on_error(std::string command_id, std::string &peer_id, std::string message, bool close) {
    if (close) {
        logger.error_log("[GameThread] on_error ", command_id, " ", peer_id, " ", message);
    }
    std::string result = NOTIFICATION_ERROR;
    result.replace(result.find("%s"), 2, message);
    result.replace(result.find("%s"), 2, command_id);
    send(peer_id, result, uWS::OpCode::TEXT);
    if (close) {
        send(peer_id, message, uWS::OpCode::CLOSE);
    }
}

void GameThread::on_lobby_call(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &data) {
    logger.debug_log("[GameThread] on_lobby_call ", command_id, " ", peer.id);
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
}

void GameThread::on_quick_join(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &data) {
    logger.debug_log("[GameThread] on_quick_join ", command_id, " ", peer.id);
    if (peer.lobby_id != "") {
        return on_error(command_id, peer.id, ERROR_PEER_IS_IN_A_LOBBY);
    }
}

void GameThread::on_create_lobby(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &data) {
    logger.debug_log("[GameThread] on_create_lobby ", command_id, " ", peer.id);
    if (peer.lobby_id != "") {
        return on_error(command_id, peer.id, ERROR_PEER_IS_IN_A_LOBBY + " " + peer.lobby_id);
    }
    auto uuid = to_string(gen());
    auto small_uuid = uuid.substr(0, 8);
    peer.lobby_id = small_uuid;
    game.lobbies.emplace(small_uuid, LobbyData{
        .id = small_uuid,
        .host = peer.id,
        .game_id = peer.game_id,
        .name = decode_string_or_default(data, "name", ""),
    });
    send(peer.id, "create_lobby", uWS::OpCode::TEXT);
}

void GameThread::on_join_lobby(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &data) {
    logger.debug_log("[GameThread] on_join_lobby ", command_id, " ", peer.id);
    if (peer.lobby_id != "") {
        return on_error(command_id, peer.id, ERROR_PEER_IS_IN_A_LOBBY);
    }
}

void GameThread::on_leave_lobby(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &data) {
    logger.debug_log("[GameThread] on_leave_lobby ", command_id, " ", peer.id);
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
}

void GameThread::on_list_lobbys(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &data) {
    logger.debug_log("[GameThread] on_list_lobbys ", command_id, " ", peer.id);
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
}

void GameThread::on_chat_lobby(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &data) {
    logger.debug_log("[GameThread] on_chat_lobby ", command_id, " ", peer.id);
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
}

void GameThread::on_lobby_tags(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &data) {
    logger.debug_log("[GameThread] on_lobby_tags ", command_id, " ", peer.id);
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    if (lobby.host != peer.id) {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_HOST);
    }
}

void GameThread::on_kick_peer(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &data) {
    logger.debug_log("[GameThread] on_kick_peer ", command_id, " ", peer.id);
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    auto &lobby = game.lobbies[peer.lobby_id];
    if (lobby.host != peer.id) {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_HOST);
    }
}

void GameThread::on_user_data(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &data) {
    logger.debug_log("[GameThread] on_user_data ", command_id, " ", peer.id);
    if (peer.lobby_id != "") {
        return on_error(command_id, peer.id, ERROR_PEER_IS_IN_A_LOBBY);
    }
    if (!decode_object(command_id, peer.id, data, peer.user_data)) {
        return on_error(command_id, peer.id, ERROR_CANNOT_PARSE_JSON + " Decode user data");
    }
}

void GameThread::on_lobby_ready(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &data) {
    logger.debug_log("[GameThread] on_lobby_ready ", command_id, " ", peer.id);
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    if (peer.ready) {
        return on_error(command_id, peer.id, ERROR_PEER_IS_READY);
    }
    std::vector<AnyElement> args(2);
    args[0] = AnyElement{peer.id};
    args[1] = AnyElement{true};
    auto err = scripted_function_call(game, "_on_ready", true, args);
    if (err != "") {
        return on_error(command_id, peer.id, err);
    }
    peer.ready = true;
    std::string message = NOTIFICATION_PEER_READY;
    message.replace(message.find("%s"), 2, peer.id);
    message.replace(message.find("%s"), 2, command_id);
    for (auto lobby_peer_id : game.lobbies[peer.lobby_id].peer_ids) {
        send(lobby_peer_id, message, uWS::OpCode::TEXT);
    }
}

void GameThread::on_lobby_unready(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &data) {
    logger.debug_log("[GameThread] on_lobby_unready ", command_id, " ", peer.id);
    if (peer.lobby_id == "") {
        return on_error(command_id, peer.id, ERROR_PEER_NOT_IN_A_LOBBY);
    }
    if (!peer.ready) {
        return on_error(command_id, peer.id, ERROR_PEER_IS_NOT_READY);
    }
    std::vector<AnyElement> args(2);
    args[0] = AnyElement{peer.id};
    args[1] = AnyElement{false};
    auto err = scripted_function_call(game, "_on_ready", true, args);
    if (err != "") {
        return on_error(command_id, peer.id, err);
    }
    peer.ready = false;
    std::string message = NOTIFICATION_PEER_UNREADY;
    message.replace(message.find("%s"), 2, peer.id);
    message.replace(message.find("%s"), 2, command_id);
    for (auto lobby_peer_id : game.lobbies[peer.lobby_id].peer_ids) {
        send(lobby_peer_id, message, uWS::OpCode::TEXT);
    }
}

void GameThread::on_seal_lobby(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &data) {
    logger.debug_log("[GameThread] on_seal_lobby ", command_id, " ", peer.id);
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
    std::vector<AnyElement> args(2);
    args[0] = AnyElement{peer.id};
    args[1] = AnyElement{true};
    auto err = scripted_function_call(game, "_on_seal", true, args);
    if (err != "") {
        return on_error(command_id, peer.id, err);
    }
    lobby.sealed = true;
    std::string message = NOTIFICATION_LOBBY_SEALED;
    message.replace(message.find("%s"), 2, peer.id);
    message.replace(message.find("%s"), 2, command_id);
    for (auto lobby_peer_id : game.lobbies[peer.lobby_id].peer_ids) {
        send(lobby_peer_id, message, uWS::OpCode::TEXT);
    }
}

void GameThread::on_unseal_lobby(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &data) {
    logger.debug_log("[GameThread] on_unseal_lobby ", command_id, " ", peer.id);
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
    std::vector<AnyElement> args(2);
    args[0] = AnyElement{peer.id};
    args[1] = AnyElement{false};
    auto err = scripted_function_call(game, "_on_seal", true, args);
    if (err != "") {
        return on_error(command_id, peer.id, err);
    }
    lobby.sealed = false;
    std::string message = NOTIFICATION_LOBBY_UNSEALED;
    message.replace(message.find("%s"), 2, peer.id);
    message.replace(message.find("%s"), 2, command_id);
    for (auto lobby_peer_id : game.lobbies[peer.lobby_id].peer_ids) {
        send(lobby_peer_id, message, uWS::OpCode::TEXT);
    }
}

std::string GameThread::scripted_function_call(GameData &game, std::string funcname, bool override, std::vector<AnyElement> &args) {
    if (game.lua.L != nullptr) {
        
    }
    if (game.angelscript.engine != nullptr) {
        asIScriptContext *ctx = game.angelscript.engine->CreateContext();
        ctx->Prepare(game.angelscript.func);
        ctx->Execute();
        if (!ctx) {
            return "Failed to create AngelScript context";
        }
        ctx->Release();
    }
    return EMPTY_STRING;
}

std::string GameThread::decode_string_or_default(simdjson::ondemand::object &object, std::string key, std::string default_value) {
    std::string_view result;
    if (object[key].get_string().get(result) != simdjson::SUCCESS) {
        return default_value;
    }
    return std::string(result);
}

int GameThread::decode_int_or_default(simdjson::ondemand::object &object, std::string key, int default_value) {
    int result;
    simdjson::ondemand::number value_number;
    if (object[key].get_number().get(value_number) != simdjson::SUCCESS) {
        return default_value;
    }
    switch (value_number.get_number_type()) {
        case simdjson::ondemand::number_type::signed_integer:
            result = value_number.get_int64();
            break;
        case simdjson::ondemand::number_type::unsigned_integer:
            result = value_number.get_uint64();
            break;
        case simdjson::ondemand::number_type::floating_point_number:
            result = value_number.get_double();
            break;
        case simdjson::ondemand::number_type::big_integer:
            result = value_number.get_int64();
            break;
    }
    return result;
}

bool GameThread::decode_value(std::string command_id, std::string &peer_id, simdjson::ondemand::value &value, AnyElement &element) {
    simdjson::error_code value_error;
    simdjson::ondemand::json_type value_type;
    bool value_bool;
    simdjson::ondemand::number value_number;
    std::string_view string_value;
    simdjson::ondemand::object child_object;
    simdjson::ondemand::array child_array;
    simdjson::ondemand::value child_array_value;

    std::vector<AnyElement> child_vector = {};
    std::unordered_map<std::string, AnyElement> child_dict = {};
    
    switch (value_type) {
        case simdjson::ondemand::json_type::array:
            value_error = value.get_array().get(child_array);
            if (value_error != simdjson::SUCCESS) {
                on_error(command_id, peer_id, ERROR_CANNOT_PARSE_JSON + " Decode value array " + std::string(simdjson::error_message(value_error)));
                return false;
            };

            for (auto array_element : child_array) {
                value_error = array_element.get(child_array_value);
                if (value_error != simdjson::SUCCESS) {
                    on_error(command_id, peer_id, ERROR_CANNOT_PARSE_JSON + " Decode value array child" + std::string(simdjson::error_message(value_error)));
                    return false;
                }
                AnyElement child_element;
                if (!decode_value(command_id, peer_id, child_array_value, child_element)) {
                    return false;
                }
                child_vector.push_back(child_element);
            }
            if (!decode_object(command_id, peer_id, child_object, child_dict)) {
                return false;
            }
            element.value = VariantElement{
                child_dict
            };
        case simdjson::ondemand::json_type::object:
            value_error = value.get_object().get(child_object);
            if (value_error != simdjson::SUCCESS) {
                on_error(command_id, peer_id, ERROR_CANNOT_PARSE_JSON + " Decode value object " + std::string(simdjson::error_message(value_error)));
                return false;
            };
            if (!decode_object(command_id, peer_id, child_object, child_dict)) {
                return false;
            }
            element.value = VariantElement{
                child_dict
            };
            break;
        case simdjson::ondemand::json_type::string:
            value_error = value.get_string().get(string_value);
            if (value_error != simdjson::SUCCESS) {
                on_error(command_id, peer_id, ERROR_CANNOT_PARSE_JSON + " Decode value string " + std::string(simdjson::error_message(value_error)));
                return false;
            };
            element.value = VariantElement{
                std::string(string_value)
            };
            break;
        case simdjson::ondemand::json_type::number:
            value_error = value.get_number().get(value_number);
            if (value_error != simdjson::SUCCESS) {
                on_error(command_id, peer_id, ERROR_CANNOT_PARSE_JSON + " Decode value number " + std::string(simdjson::error_message(value_error)));
                return false;
            };
            switch (value_number.get_number_type()) {
                case simdjson::ondemand::number_type::floating_point_number:
                    element.value = VariantElement{
                        value_number.get_double()
                    };
                    break;
                case simdjson::ondemand::number_type::signed_integer:
                    element.value = VariantElement{
                        value_number.get_int64()
                    };
                    break;
                case simdjson::ondemand::number_type::unsigned_integer:
                    element.value = VariantElement{
                        static_cast<int64_t>(value_number.get_uint64())
                    };
                    break;
                case simdjson::ondemand::number_type::big_integer:
                    element.value = VariantElement{
                        value_number.get_double()
                    };
                    break;
            }
            break;
        case simdjson::ondemand::json_type::boolean:
            value_error = value.get_bool().get(value_bool);
            if (value_error != simdjson::SUCCESS) {
                on_error(command_id, peer_id, ERROR_CANNOT_PARSE_JSON + " Decode value bool " + std::string(simdjson::error_message(value_error)));
                return false;
            };
            element.value = VariantElement{
                value_bool
            };
            break;
        case simdjson::ondemand::json_type::null:
            element.value = VariantElement{
                std::monostate{}
            };
            break;
        default:
            on_error(command_id, peer_id, ERROR_CANNOT_PARSE_JSON + " Unexpected value type ");
            return false;
    }
    return true;
}

bool GameThread::decode_object(std::string command_id, std::string &peer_id, simdjson::ondemand::object &object, std::unordered_map<std::string, AnyElement> &dict) {
    static std::string_view key_string;
    static simdjson::error_code value_error;
    simdjson::ondemand::value value_any;
    for (auto field : object) {
        value_error = field.escaped_key().get(key_string);
        if (value_error != simdjson::SUCCESS) {
            on_error(command_id, peer_id, ERROR_CANNOT_PARSE_JSON + " Decode map key " + std::string(simdjson::error_message(value_error)));
            return false;
        }
        value_error = field.value().get(value_any);
        if (value_error != simdjson::SUCCESS) {
            on_error(command_id, peer_id, ERROR_CANNOT_PARSE_JSON + " Decode map value " + std::string(simdjson::error_message(value_error)));
            return false;
        };
        AnyElement any_element;
        if (!decode_value(command_id, peer_id, value_any, any_element)) {
            return false;
        }
        dict[std::string(key_string)] = any_element;
    }
    return true;
}
