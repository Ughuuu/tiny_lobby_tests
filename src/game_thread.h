#pragma once
#include "server_logger.h"
#include <readerwriterqueue.h>
#include "websocket_server.h"
#include "App.h"
#include "peer_data.h"
#include "lobby_data.h"
#include <atomic>
#include <simdjson.h>

class GameThread {
    ServerLogger logger;
    std::unordered_map<std::string, PeerData> peers;
    std::unordered_map<std::string, std::string> reconnection_to_peer_ids;
    std::unordered_map<std::string, LobbyData> lobbies;
    // add them both so we don't need template
    WebSocketServer<true> *webserver_ssl;
    WebSocketServer<false> *webserver_nossl;
    moodycamel::BlockingReaderWriterQueue<WebSocketMessage> &message_queue;
    struct uWS::Loop *loop;
public:
    void run();
    void on_connect(std::string &peer_id, std::string &game_id, std::string &reconnection_id);
    void on_close(std::string &peer_id);
    void on_error(std::string &peer_id, std::string &message);

    void on_lobby_call(std::string &peer_id, simdjson::ondemand::document &doc);
    void on_quick_join(std::string &peer_id, simdjson::ondemand::document &doc);
    void on_create_lobby(std::string &peer_id, simdjson::ondemand::document &doc);
    void on_join_lobby(std::string &peer_id, simdjson::ondemand::document &doc);
    void on_leave_lobby(std::string &peer_id, simdjson::ondemand::document &doc);
    void on_list_lobbys(std::string &peer_id, simdjson::ondemand::document &doc);
    void on_chat_lobby(std::string &peer_id, simdjson::ondemand::document &doc);
    void on_lobby_tags(std::string &peer_id, simdjson::ondemand::document &doc);
    void on_kick_peer(std::string &peer_id, simdjson::ondemand::document &doc);
    void on_user_data(std::string &peer_id, simdjson::ondemand::document &doc);
    void on_lobby_ready(std::string &peer_id, simdjson::ondemand::document &doc);
    void on_lobby_unready(std::string &peer_id, simdjson::ondemand::document &doc);
    void on_seal_lobby(std::string &peer_id, simdjson::ondemand::document &doc);
    void on_unseal_lobby(std::string &peer_id, simdjson::ondemand::document &doc);

    void send(std::string &peer_id, const std::string &message, uWS::OpCode opCode = uWS::OpCode::TEXT);
    GameThread(bool verbose,
        moodycamel::BlockingReaderWriterQueue<WebSocketMessage> &message_queue,
        uWS::Loop *loop,
        WebSocketServer<true> *webserver,
        WebSocketServer<false> *webserver_nossl);
};
