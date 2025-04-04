#pragma once
#include <readerwriterqueue.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "App.h"
#include "game_data.h"
#include "lobby_data.h"
#include "peer_data.h"
#include "server_logger.h"
#include "websocket_server.h"
#include "yyjson.h"

class GameThread {
    int messages_sent = 0;
    int messages_received = 0;
    int max_reconnection_time = 6 * 60 * 1000;
    int listing_interval;
    int check_interval = 100;
    int64_t now = 0;
    ServerLogger logger;
    std::string logs_folder;
    std::string scripts_folder;
    boost::uuids::random_generator gen;
    moodycamel::BlockingReaderWriterQueue<WebSocketReceivedMessage> &receive_queue;
    struct uWS::Loop *loop;
    WebSocketServer<true> *webserver_ssl;
    WebSocketServer<false> *webserver_nossl;

   public:
    int64_t get_time() { return now; }
    boost::container::flat_map<std::string, GameData> games;
    void load_games();
    void unload_games();
    void run();
    void handle_events();
    void handle_disconnects();
    void handle_lobby_list();
    void handle_timers();
    void handle_send();
    void handle_tick();
    void on_connect(GameData &game, std::string &peer_id, std::string &game_id,
                    std::string &reconnection_token);
    void on_close(GameData &game, std::string &peer_id);
    void on_error(GameData &game, std::string command_id, std::string peer_id, std::string message,
                  bool close = false, bool logical_error = false);

    // relay lobby functions

    void on_lobby_data(GameData &game, std::string command_id, PeerData &peer,
                       yyjson_val *data_val);
    void on_data_to(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);
    void on_data_to_all(GameData &game, std::string command_id, PeerData &peer,
                        yyjson_val *data_val);
    void on_notify_to(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);
    void on_lobby_notify(GameData &game, std::string command_id, PeerData &peer,
                         yyjson_val *data_val);

    // scripted lobby functions
    void on_lobby_call(GameData &game, std::string command_id, PeerData &peer,
                       yyjson_val *data_val);
    void on_quick_join(GameData &game, std::string command_id, PeerData &peer,
                       yyjson_val *data_val);
    void on_create_lobby(GameData &game, std::string command_id, PeerData &peer,
                         yyjson_val *data_val);
    bool on_join_lobby(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val,
                       std::string lobby_id_override = "");
    void on_leave_lobby(GameData &game, std::string command_id, PeerData &peer,
                        yyjson_val *data_val);
    void on_list_lobby(GameData &game, std::string command_id, PeerData &peer,
                       yyjson_val *data_val);
    void on_chat_lobby(GameData &game, std::string command_id, PeerData &peer,
                       yyjson_val *data_val);
    void on_lobby_tags(GameData &game, std::string command_id, PeerData &peer,
                       yyjson_val *data_val);
    void on_kick_peer(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);
    void on_user_data(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val);
    void on_lobby_ready(GameData &game, std::string command_id, PeerData &peer,
                        yyjson_val *data_val);
    void on_lobby_unready(GameData &game, std::string command_id, PeerData &peer,
                          yyjson_val *data_val);
    void on_seal_lobby(GameData &game, std::string command_id, PeerData &peer,
                       yyjson_val *data_val);
    void on_unseal_lobby(GameData &game, std::string command_id, PeerData &peer,
                         yyjson_val *data_val);

    void set_lobby_sealed(GameData &game, LobbyData &lobby, std::string peer_id,
                          std::string command_id, bool sealed);
    void set_lobby_ready(GameData &game, LobbyData &lobby, PeerData &peer, std::string command_id,
                         bool ready);

    void remove_peer_from_lobby(GameData &game, LobbyData &lobby, std::string &peer_id,
                                const std::string &command_id, bool kicked = false);

    AnyElement scripted_function_call(std::string peer_id, std::string lobby_id, GameData &game,
                                      std::string funcname, bool override,
                                      boost::container::vector<AnyElement> &args, bool &has_error);

    AnyElement decode_luatable(lua_State *L, int idx);
    AnyElement decode_luavalue(lua_State *L, int idx);

    void send_message(GameData &game, std::string &peer_id, std::string &message);
    void notify_lobby_changes(GameData &game, std::string &lobby_id);
    void notify_peer(GameData &game, std::string &lobby_id, std::string peer_id,
                     std::string notification);
    void send(GameData &game, const std::string &peer_id, const std::string &message,
              uWS::OpCode opCode = uWS::OpCode::TEXT);
    void send_all(GameData &game);
    GameThread(bool verbose, std::string log_folder, std::string scripts_folder,
               moodycamel::BlockingReaderWriterQueue<WebSocketReceivedMessage> &receive_queue,
               uWS::Loop *loop, WebSocketServer<true> *webserver,
               WebSocketServer<false> *webserver_nossl, int listing_interval,
               int max_recconection_time);
};
