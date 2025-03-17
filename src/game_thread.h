#pragma once
#include "server_logger.h"
#include <readerwriterqueue.h>
#include "websocket_server.h"
#include "App.h"
#include "peer_data.h"
#include "yyjson.h"
#include "game_data.h"
#include "lobby_data.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

class GameThread {
    int MAX_RECONNECTION_TIME = 6 * 60 * 1000;
    ServerLogger logger;
    std::string logs_folder;
    std::string scripts_folder;
    boost::uuids::random_generator gen;
    // add them both so we don't need template
    WebSocketServer<true> *webserver_ssl;
    WebSocketServer<false> *webserver_nossl;
    moodycamel::BlockingReaderWriterQueue<WebSocketMessage> &message_queue;
    struct uWS::Loop *loop;
public:
    std::unordered_map<std::string, GameData> games;
    void load_games();
    void unload_games();
    void run();
    void handle_events();
    void handle_disconnects();
    void on_connect(GameData &game, std::string &peer_id, std::string &game_id, std::string &reconnection_token);
    void on_close(GameData &game, std::string &peer_id);
    void on_error(std::string command_id, std::string &peer_id, std::string message, bool close = false);

    void on_lobby_call(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);
    void on_quick_join(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);
    void on_create_lobby(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);
    void on_join_lobby(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);
    void on_leave_lobby(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);
    void on_list_lobbys(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);
    void on_chat_lobby(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);
    void on_lobby_tags(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);
    void on_kick_peer(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);
    void on_user_data(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);
    void on_lobby_ready(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);
    void on_lobby_unready(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);
    void on_seal_lobby(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);
    void on_unseal_lobby(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);

    void remove_peer_from_lobby(GameData &game, std::string peer_id);

    AnyElement scripted_function_call(std::string &lobby_id, GameData &game, std::string funcname, bool override, std::vector<AnyElement> &args, bool &has_error);

    AnyElement decode_luatable(lua_State *L, int idx);
    AnyElement decode_luavalue(lua_State *L, int idx);

    std::string decode_array(yyjson_val *array, AnyElement &element);
    std::string decode_object(yyjson_val *object, std::unordered_map<std::string, AnyElement> &dict);
    std::string decode_value(yyjson_val *value, AnyElement &element);

    std::string decode_string_or_default(yyjson_val *object, std::string key, std::string default_value);
    int decode_int_or_default(yyjson_val *object, std::string key, int default_value);

    void send(std::string &peer_id, const std::string &message, uWS::OpCode opCode = uWS::OpCode::TEXT);
    GameThread(bool verbose,
        std::string log_folder,
        std::string scripts_folder,
        moodycamel::BlockingReaderWriterQueue<WebSocketMessage> &message_queue,
        uWS::Loop *loop,
        WebSocketServer<true> *webserver,
        WebSocketServer<false> *webserver_nossl);
};
