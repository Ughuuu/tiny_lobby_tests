#pragma once
#include <readerwriterqueue.h>
#include <uwebsockets/App.h>

#include <atomic>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <efsw/efsw.hpp>
#include <mutex>

#include "game_data.h"
#include "games_listener.h"
#include "lobby_data.h"
#include "peer_data.h"
#include "pogr_client.h"
#include "server_logger.h"
#include "websocket_server.h"
#include "yyjson.h"

static std::vector<std::string> get_expected_functions(){
    std::vector<std::string> expected_functions = {"_can_create", "_on_create", "_on_join",
        "_on_chat",    "_on_tags",   "_on_kick",
        "_on_ready",   "_on_seal",   "_on_left",
        "_can_resize", "_on_resize", "_on_title"};
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
    moodycamel::BlockingReaderWriterQueue<AnalyticsEvent> &analytics_queue;
    moodycamel::BlockingReaderWriterQueue<WebSocketReceivedMessage> &receive_queue;
    struct uWS::Loop *loop;
    WebSocketServer<true> *webserver_ssl;
    WebSocketServer<false> *webserver_nossl;
    std::atomic<bool> &stop;
    GamesListener games_listener;
    efsw::FileWatcher file_watcher;
    moodycamel::ReaderWriterQueue<std::string> file_watcher_queue;

   public:
    int64_t get_time() { return now; }
    boost::container::flat_map<std::string, GameData> games;
    void load_games();
    void unload_games();
    void reload_game(std::string folder_name);
    void run();
    void time_run();
    bool handle_events();
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

    void send_message(GameData &game, std::string &peer_id, std::string &message);
    void notify_lobby_changes(GameData &game, std::string &lobby_id);
    void notify_peer(GameData &game, std::string &lobby_id, std::string peer_id,
                     const AnyElement &notification);
    void send(GameData &game, const std::string &peer_id, const std::string &message,
              uWS::OpCode opCode = uWS::OpCode::TEXT);
    void send_all(GameData &game);
    GameThread(bool verbose, std::string log_folder, std::string scripts_folder,
               moodycamel::BlockingReaderWriterQueue<AnalyticsEvent> &analytics_queue,
               moodycamel::BlockingReaderWriterQueue<WebSocketReceivedMessage> &receive_queue,
               uWS::Loop *loop, WebSocketServer<true> *webserver,
               WebSocketServer<false> *webserver_nossl, std::atomic<bool> &stop,
               int listing_interval, int max_recconection_time);
};
