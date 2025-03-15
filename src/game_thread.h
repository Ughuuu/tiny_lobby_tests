#pragma once
#include "server_logger.h"
#include <readerwriterqueue.h>
#include "websocket_server.h"
#include "App.h"
#include "peer_data.h"
#include "lobby_data.h"
#include "game_data.h"
#include <simdjson.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

class GameThread {
    ServerLogger logger;
    std::string scripts_folder;
    boost::uuids::random_generator gen;
    std::unordered_map<std::string, GameData> games;
    // add them both so we don't need template
    WebSocketServer<true> *webserver_ssl;
    WebSocketServer<false> *webserver_nossl;
    moodycamel::BlockingReaderWriterQueue<WebSocketMessage> &message_queue;
    struct uWS::Loop *loop;
public:
    void load_games();
    void unload_games();
    void run();
    void on_connect(GameData &game, std::string &peer_id, std::string &game_id);
    void on_close(GameData &game, std::string &peer_id);
    void on_error(std::string command_id, std::string &peer_id, std::string message, bool close = false);

    void on_lobby_call(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &doc);
    void on_quick_join(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &doc);
    void on_create_lobby(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &doc);
    void on_join_lobby(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &doc);
    void on_leave_lobby(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &doc);
    void on_list_lobbys(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &doc);
    void on_chat_lobby(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &doc);
    void on_lobby_tags(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &doc);
    void on_kick_peer(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &doc);
    void on_user_data(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &doc);
    void on_lobby_ready(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &doc);
    void on_lobby_unready(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &doc);
    void on_seal_lobby(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &doc);
    void on_unseal_lobby(GameData &game, std::string command_id, PeerData &peer, simdjson::ondemand::object &doc);

    std::string scripted_function_call(GameData &game, std::string funcname, bool override, std::vector<AnyElement> &args);

    bool decode_object(std::string command_id, std::string &peer_id, simdjson::ondemand::object &object, std::unordered_map<std::string, AnyElement> &dict);
    bool decode_value(std::string command_id, std::string &peer_id, simdjson::ondemand::value &value, AnyElement &element);

    std::string decode_string_or_default(simdjson::ondemand::object &object, std::string key, std::string default_value);
    int decode_int_or_default(simdjson::ondemand::object &object, std::string key, int default_value);

    void send(std::string &peer_id, const std::string &message, uWS::OpCode opCode = uWS::OpCode::TEXT);
    GameThread(bool verbose,
        std::string scripts_folder,
        moodycamel::BlockingReaderWriterQueue<WebSocketMessage> &message_queue,
        uWS::Loop *loop,
        WebSocketServer<true> *webserver,
        WebSocketServer<false> *webserver_nossl);
};
