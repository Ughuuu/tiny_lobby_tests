#pragma once
#include <angelscript.h>

#include <string>
#include <vector>

#include "any_type.h"
#include "server_logger.h"

class GameThread;
class CScriptArray;
class CScriptAny;

struct LobbyAS {
    GameThread* game_thread;
    std::string game_id;
    std::string lobby_id;
    std::string peer_id;
};

struct ScriptAS {
    bool autoreload;
    std::string scripts_folder;
    std::string folder_name;
    std::string script_entrypoint;
    std::string logs_folder;
    GameThread* game_thread;

    asIScriptEngine* as_engine = nullptr;
    asIScriptContext* as_context = nullptr;
    asIScriptModule* as_module = nullptr;
    boost::container::flat_map<std::string, int> as_functions;
    std::string as_peer_id;
    std::string as_lobby_id;
    std::string as_game_id;

   public:
    bool enabled = false;
    AnyElement func_call(std::string& func_name, boost::container::vector<AnyElement>& args,
                         std::string& peer_id, std::string& lobby_id, std::string& game_id,
                         bool& has_error);
    boost::container::flat_set<std::string> open();
    void as_start_timer(std::string& timer_id, int duration, CScriptArray* arg);
    void as_stop_timer(std::string& timer_id);
    void as_notifty(std::string& peer_id, CScriptAny* message);
    void as_broadcast_chat(std::string& message);
    void close();

    std::string get_lobby_id() { return as_lobby_id; }
    std::string get_game_id() { return as_game_id; }
    std::string get_calling_peer_id() { return as_peer_id; }
};
