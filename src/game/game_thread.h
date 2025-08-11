#pragma once
#include <readerwriterqueue.h>
#include <uwebsockets/App.h>

#include <atomic>
#include <boost/container/map.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <efsw/efsw.hpp>

#include "../common/any_type.h"
#include "../common/server_logger.h"
#include "../database/database_thread.h"
#include "../websocket/websocket_server.h"
#include "game_data.h"
#include "games_listener.h"
#include "lobby_data.h"
#include "peer_data.h"
#include "yyjson.h"

static std::vector<std::string> get_expected_functions() {
    std::vector<std::string> expected_functions = {
        "_on_peer_connected", "_on_peer_disconnected", "_can_create_lobby", "_on_lobby_created",
        "_can_peer_join",     "_on_peer_joined",       "_can_peer_chat",    "_can_host_set_tags",
        "_can_host_kick",     "_can_peer_ready",       "_can_host_seal",    "_on_peer_leave",
        "_can_host_resize",   "_can_host_set_title",   "_on_lobby_tick",    "_on_server_reload",
        "_on_server_init"};
    return expected_functions;
}

class GameThread {
    int messages_sent = 0;
    int messages_received = 0;
    int max_reconnection_time = 6 * 60 * 1000;
    int listing_interval;
    int check_time = 100;
    std::atomic<int64_t> now;
    ServerLogger logger;
    std::string logs_folder;
    std::string scripts_folder;
    boost::uuids::random_generator gen;
    moodycamel::BlockingReaderWriterQueue<WebSocketReceivedMessage> &receive_queue;
    struct uWS::Loop *loop;
    WebSocketServer *webserver;
    std::atomic<bool> &stop;
    GamesListener games_listener;
    efsw::FileWatcher file_watcher;
    moodycamel::ReaderWriterQueue<std::string> file_watcher_queue;
    moodycamel::BlockingReaderWriterQueue<DatabaseReceivedMessage> &database_queue;
    bool db_enabled;
    DatabaseThread database_thread;

   public:
    boost::container::map<std::string, GameData> games;
    int64_t get_time() { return now; }

    void load_games();
    void unload_game(std::string game_id);
    void reload_game(std::string folder_name);
    void run();
    void time_run();
    void handle_afk();
    bool handle_events();
    void handle_disconnects();
    void handle_lobby_list();
    void handle_timers();
    void handle_send();
    void handle_tick();
    void on_connect(GameData &game, std::string &peer_id, std::string &game_id,
                    std::string &reconnection_token, std::string &platform,
                    std::string &platform_id, std::string &name);
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
    void on_create_lobby(
        GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val,
        boost::container::flat_map<std::string, AnyElement> *previous_tags = nullptr);
    bool on_join_lobby(GameData &game, std::string command_id, PeerData &peer, yyjson_val *data_val,
                       std::string lobby_id_override = "");
    void on_leave_lobby(GameData &game, std::string command_id, PeerData &peer,
                        yyjson_val *data_val);
    void on_list_lobby(GameData &game, std::string command_id, PeerData &peer,
                       yyjson_val *data_val);
    void on_stop_list_lobby(GameData &game, std::string command_id, PeerData &peer,
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
    void on_lobby_max_players(GameData &game, std::string command_id, PeerData &peer,
                              yyjson_val *data_val);
    void on_lobby_title(GameData &game, std::string command_id, PeerData &peer,
                        yyjson_val *data_val);
    void on_lobby_password(GameData &game, std::string command_id, PeerData &peer,
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

    void send_message(GameData &game, std::string &peer_id, std::string &message,
                      boost::container::flat_map<std::string, AnyElement> &chat_metadata);
    void kick_peer(GameData &game, std::string &lobby_id, std::string &peer_id);
    void notify_lobby_changes(GameData &game, std::string &lobby_id);
    void notify_peer(GameData &game, std::string &lobby_id, std::string peer_id,
                     const AnyElement &notification);
    void notify_all(GameData &game, std::string &lobby_id, const AnyElement &notification);
    void send(GameData &game, const std::string &peer_id, const std::string &message,
              uWS::OpCode opCode = uWS::OpCode::TEXT);
    void set_lobby_host(GameData &game, LobbyData &lobby, const std::string &new_host);
    void leaderboards_set_score(GameData &game, const std::string &peer_id,
                                const std::string &leaderboard_id, int64_t score,
                                const std::string &leaderboard_type);

    GameThread(bool db_enabled, bool verbose, std::string log_folder, std::string scripts_folder,
               moodycamel::BlockingReaderWriterQueue<WebSocketReceivedMessage> &receive_queue,
               moodycamel::BlockingReaderWriterQueue<DatabaseReceivedMessage> &database_queue,
               uWS::Loop *loop, WebSocketServer *webserver, std::atomic<bool> &stop,
               int listing_interval, int max_recconection_time);

   private:
    void handle_command(GameData &game, int command, const std::string &command_id, PeerData &peer,
                        yyjson_val *data_val);
};
