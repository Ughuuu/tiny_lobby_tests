#include "game_server.h"
#include "lua.hpp"
#include <angelscript.h>

GameServer::GameServer(bool verbose,
    moodycamel::BlockingReaderWriterQueue<WebSocketMessage> &message_queue,
    uWS::Loop *loop,
    WebSocketServer *webserver): logger(verbose), message_queue(message_queue), loop(loop), webserver(webserver) {
    logger.debug_log("[GameServer] on_start");
}

void GameServer::run() {
    // max 10kb
    simdjson::ondemand::parser parser(10000);
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    asIScriptEngine *engine = asCreateScriptEngine();
    if (!engine) {
        std::cerr << "Failed to create AngelScript engine" << std::endl;
        return;
    }
    asIScriptModule *mod = engine->GetModule("default", asGM_ALWAYS_CREATE);
    const char *script = 
    "void main() {"
    "   print('Hello from AngelScript!');"
    "}";

    mod->AddScriptSection("my_script", script, strlen(script), 0);
    mod->Build();

    asIScriptContext *ctx = engine->CreateContext();

    asIScriptFunction *func = mod->GetFunctionByName("main");
    ctx->Prepare(func);
    ctx->Execute();

    ctx->Release();
    logger.debug_log("[GameServer] on_start_thread");

    simdjson::ondemand::document doc;
    while (true) {
        WebSocketMessage message;
        message_queue.wait_dequeue(message);
        simdjson::padded_string padded_string(message.message);
        auto error = parser.iterate(padded_string).get(doc);
        if(error != simdjson::SUCCESS) {
            std::string error_message = "Cannot parse json. ";
            error_message += message.message + " ";
            error_message += simdjson::error_message(error);
            on_error(message.id,  error_message);
            continue;
        }
        if (peers.find(message.id) == peers.end()) {
            peers[message.id] = PeerData{
                .id = message.id,
                .game_id = message.game_id,
                .reconnection_id = message.reconnection_id
            };
        }
        switch (message.event) {
            case WebSocketEvent::OPEN:
                on_connect(message.id);
                break;
            case WebSocketEvent::CLOSE:
                on_close(message.id);
                break;
            case WebSocketEvent::MESSAGE:
                std::string_view command;
                auto error = doc.get_object()["command"].get_string().get(command);
                if (error != simdjson::SUCCESS) {
                    std::string error_message = "Cannot get command from ";
                    error_message += message.message;
                    error_message += " ";
                    error_message += simdjson::error_message(error);
                    on_error(message.id,  error_message);
                    break;
                }
                if (command == "lobby_call") {
                    on_lobby_call(message.id, doc);
                } else if (command == "quick_join") {
                    on_quick_join(message.id, doc);
                } else if (command == "create_lobby") {
                    on_create_lobby(message.id, doc);
                } else if (command == "join_lobby") {
                    on_join_lobby(message.id, doc);
                } else if (command == "leave_lobby") {
                    on_leave_lobby(message.id, doc);
                } else if (command == "list_lobbys") {
                    on_list_lobbys(message.id, doc);
                } else if (command == "chat_lobby") {
                    on_chat_lobby(message.id, doc);
                } else if (command == "lobby_tags") {
                    on_lobby_tags(message.id, doc);
                } else if (command == "kick_peer") {
                    on_kick_peer(message.id, doc);
                } else if (command == "user_data") {
                    on_user_data(message.id, doc);
                } else if (command == "lobby_ready") {
                    on_lobby_ready(message.id, doc);
                } else if (command == "lobby_unready") {
                    on_lobby_unready(message.id, doc);
                } else if (command == "seal_lobby") {
                    on_seal_lobby(message.id, doc);
                } else if (command == "unseal_lobby") {
                    on_unseal_lobby(message.id, doc);
                } else {
                    std::string error_message = "Unknown command: ";
                    error_message += command;
                    on_error(message.id,  error_message);
                }
                
                break;
        }
    }
    lua_close(L);
    engine->Release();
}

void GameServer::send(std::string &peer_id, const std::string &message, uWS::OpCode opCode) {
    loop->defer([id = peer_id, msg = message, webserver = webserver]() {
        webserver->send(id, msg, uWS::OpCode::TEXT);
    });
}

void GameServer::on_connect(std::string &peer_id) {
    logger.debug_log("[GameServer] on_connect");
}

void GameServer::on_close(std::string &peer_id) {
    logger.debug_log("[GameServer] on_close");
    peers.erase(peer_id);
}

void GameServer::on_error(std::string &peer_id, std::string &message) {
    logger.debug_log("[GameServer] on_error ", message);
    send(message, message, uWS::OpCode::CLOSE);
    on_close(peer_id);
}

void GameServer::on_lobby_call(std::string &peer_id, simdjson::ondemand::document &doc) {
    logger.debug_log("[GameServer] on_lobby_call");
}

void GameServer::on_quick_join(std::string &peer_id, simdjson::ondemand::document &doc) {
    logger.debug_log("[GameServer] on_quick_join");
    send(peer_id, "quick_join", uWS::OpCode::TEXT);
}

void GameServer::on_create_lobby(std::string &peer_id, simdjson::ondemand::document &doc) {
    logger.debug_log("[GameServer] on_create_lobby");
    send(peer_id, "create_lobby", uWS::OpCode::TEXT);
}

void GameServer::on_join_lobby(std::string &peer_id, simdjson::ondemand::document &doc) {
    logger.debug_log("[GameServer] on_join_lobby");
}

void GameServer::on_leave_lobby(std::string &peer_id, simdjson::ondemand::document &doc) {
    logger.debug_log("[GameServer] on_leave_lobby");
}

void GameServer::on_list_lobbys(std::string &peer_id, simdjson::ondemand::document &doc) {
    logger.debug_log("[GameServer] on_list_lobbys");
}

void GameServer::on_chat_lobby(std::string &peer_id, simdjson::ondemand::document &doc) {
    logger.debug_log("[GameServer] on_chat_lobby");
}

void GameServer::on_lobby_tags(std::string &peer_id, simdjson::ondemand::document &doc) {
    logger.debug_log("[GameServer] on_lobby_tags");
}

void GameServer::on_kick_peer(std::string &peer_id, simdjson::ondemand::document &doc) {
    logger.debug_log("[GameServer] on_kick_peer");
}

void GameServer::on_user_data(std::string &peer_id, simdjson::ondemand::document &doc) {
    logger.debug_log("[GameServer] on_user_data");
}

void GameServer::on_lobby_ready(std::string &peer_id, simdjson::ondemand::document &doc) {
    logger.debug_log("[GameServer] on_lobby_ready");
}

void GameServer::on_lobby_unready(std::string &peer_id, simdjson::ondemand::document &doc) {
    logger.debug_log("[GameServer] on_lobby_unready");
}

void GameServer::on_seal_lobby(std::string &peer_id, simdjson::ondemand::document &doc) {
    logger.debug_log("[GameServer] on_seal_lobby");
}

void GameServer::on_unseal_lobby(std::string &peer_id, simdjson::ondemand::document &doc) {
    logger.debug_log("[GameServer] on_unseal_lobby");
}
