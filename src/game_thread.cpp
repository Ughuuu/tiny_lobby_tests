#include "game_thread.h"
#include "lua.hpp"
#include <angelscript.h>


std::string ERROR_CANNOT_PARSE_COMMAND = "Cannot parse command from json";
std::string ERROR_CANNOT_PARSE_JSON = "Cannot parse json";
std::string ERROR_PEER_NOT_FOUND = "Peer not found";
std::string ERROR_PEER_ALREADY_EXISTS = "Peer already exists";

GameThread::GameThread(bool verbose,
    moodycamel::BlockingReaderWriterQueue<WebSocketMessage> &message_queue,
    uWS::Loop *loop,
    WebSocketServer<true> *webserver,
    WebSocketServer<false> *webserver_no_ssl): logger(verbose), message_queue(message_queue), loop(loop), webserver_ssl(webserver), webserver_nossl(webserver_no_ssl) {
    logger.debug_log("[GameThread] on_start");
}

void GameThread::run() {
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
    logger.debug_log("[GameThread] on_start_thread");

    simdjson::ondemand::document doc;
    while (true) {
        WebSocketMessage message;
        message_queue.wait_dequeue(message);
        switch (message.event) {
            case WebSocketEvent::OPEN:
                on_connect(message.id, message.game_id, message.reconnection_id);
                break;
            case WebSocketEvent::CLOSE:
                on_close(message.id);
                break;
            case WebSocketEvent::MESSAGE:
                // decode message
                simdjson::padded_string padded_string(message.message);
                auto parser_error = parser.iterate(padded_string).get(doc);
                if(parser_error != simdjson::SUCCESS) {
                    std::string error_message = ERROR_CANNOT_PARSE_JSON + " " + simdjson::error_message(parser_error);
                    on_error(message.id,  error_message);
                    continue;
                }
                // get command
                std::string_view command;
                auto value_error = doc.get_object()["command"].get_string().get(command);
                if (value_error != simdjson::SUCCESS) {
                    std::string error_message = ERROR_CANNOT_PARSE_COMMAND + " " + simdjson::error_message(value_error);
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

void GameThread::on_connect(std::string &peer_id, std::string &game_id, std::string &reconnection_id) {
    logger.debug_log("[GameThread] on_connect");
    if (peers.find(peer_id) != peers.end()) {
        on_error(peer_id, ERROR_PEER_ALREADY_EXISTS);
        return;
    }
    if (reconnection_id != "") {
        if (reconnection_to_peer_ids.find(reconnection_id) == reconnection_to_peer_ids.end()) {
            // we didn't find any reconnection_id, generate a new one
            reconnection_id = std::to_string(std::rand());
        }
    }
    peers[peer_id] = PeerData{
        .id = peer_id,
        .game_id = game_id,
        .reconnection_id = reconnection_id
    };
    logger.debug_log("[GameThread] on_connect");
}

void GameThread::on_close(std::string &peer_id) {
    logger.debug_log("[GameThread] on_close");
    peers.erase(peer_id);
}

void GameThread::on_error(std::string &peer_id, std::string &message) {
    logger.error_log("[GameThread] on_error ", message);
    send(message, message, uWS::OpCode::CLOSE);
    on_close(peer_id);
}

void GameThread::on_lobby_call(std::string &peer_id, simdjson::ondemand::document &doc) {
    if (peers.find(peer_id) == peers.end()) {
        on_error(peer_id, ERROR_PEER_NOT_FOUND);
        return;
    }
    logger.debug_log("[GameThread] on_lobby_call");
}

void GameThread::on_quick_join(std::string &peer_id, simdjson::ondemand::document &doc) {
    if (peers.find(peer_id) == peers.end()) {
        on_error(peer_id, ERROR_PEER_NOT_FOUND);
        return;
    }
    logger.debug_log("[GameThread] on_quick_join");
    send(peer_id, "quick_join", uWS::OpCode::TEXT);
}

void GameThread::on_create_lobby(std::string &peer_id, simdjson::ondemand::document &doc) {
    if (peers.find(peer_id) == peers.end()) {
        on_error(peer_id, ERROR_PEER_NOT_FOUND);
        return;
    }
    logger.debug_log("[GameThread] on_create_lobby");
    send(peer_id, "create_lobby", uWS::OpCode::TEXT);
}

void GameThread::on_join_lobby(std::string &peer_id, simdjson::ondemand::document &doc) {
    if (peers.find(peer_id) == peers.end()) {
        on_error(peer_id, ERROR_PEER_NOT_FOUND);
        return;
    }
    logger.debug_log("[GameThread] on_join_lobby");
}

void GameThread::on_leave_lobby(std::string &peer_id, simdjson::ondemand::document &doc) {
    if (peers.find(peer_id) == peers.end()) {
        on_error(peer_id, ERROR_PEER_NOT_FOUND);
        return;
    }
    logger.debug_log("[GameThread] on_leave_lobby");
}

void GameThread::on_list_lobbys(std::string &peer_id, simdjson::ondemand::document &doc) {
    if (peers.find(peer_id) == peers.end()) {
        on_error(peer_id, ERROR_PEER_NOT_FOUND);
        return;
    }
    logger.debug_log("[GameThread] on_list_lobbys");
}

void GameThread::on_chat_lobby(std::string &peer_id, simdjson::ondemand::document &doc) {
    if (peers.find(peer_id) == peers.end()) {
        on_error(peer_id, ERROR_PEER_NOT_FOUND);
        return;
    }
    logger.debug_log("[GameThread] on_chat_lobby");
}

void GameThread::on_lobby_tags(std::string &peer_id, simdjson::ondemand::document &doc) {
    if (peers.find(peer_id) == peers.end()) {
        on_error(peer_id, ERROR_PEER_NOT_FOUND);
        return;
    }
    logger.debug_log("[GameThread] on_lobby_tags");
}

void GameThread::on_kick_peer(std::string &peer_id, simdjson::ondemand::document &doc) {
    if (peers.find(peer_id) == peers.end()) {
        on_error(peer_id, ERROR_PEER_NOT_FOUND);
        return;
    }
    logger.debug_log("[GameThread] on_kick_peer");
}

void GameThread::on_user_data(std::string &peer_id, simdjson::ondemand::document &doc) {
    if (peers.find(peer_id) == peers.end()) {
        on_error(peer_id, ERROR_PEER_NOT_FOUND);
        return;
    }
    logger.debug_log("[GameThread] on_user_data");
}

void GameThread::on_lobby_ready(std::string &peer_id, simdjson::ondemand::document &doc) {
    if (peers.find(peer_id) == peers.end()) {
        on_error(peer_id, ERROR_PEER_NOT_FOUND);
        return;
    }
    logger.debug_log("[GameThread] on_lobby_ready");
}

void GameThread::on_lobby_unready(std::string &peer_id, simdjson::ondemand::document &doc) {
    if (peers.find(peer_id) == peers.end()) {
        on_error(peer_id, ERROR_PEER_NOT_FOUND);
        return;
    }
    logger.debug_log("[GameThread] on_lobby_unready");
}

void GameThread::on_seal_lobby(std::string &peer_id, simdjson::ondemand::document &doc) {
    if (peers.find(peer_id) == peers.end()) {
        on_error(peer_id, ERROR_PEER_NOT_FOUND);
        return;
    }
    logger.debug_log("[GameThread] on_seal_lobby");
}

void GameThread::on_unseal_lobby(std::string &peer_id, simdjson::ondemand::document &doc) {
    if (peers.find(peer_id) == peers.end()) {
        on_error(peer_id, ERROR_PEER_NOT_FOUND);
        return;
    }
    logger.debug_log("[GameThread] on_unseal_lobby");
}
