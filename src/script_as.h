#pragma once
#include <angelscript.h>

#include <string>
#include <vector>

#include "any_type.h"
#include "server_logger.h"

class GameThread;

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

   public:
    bool enabled = false;
    AnyElement func_call(std::string& func_name, boost::container::vector<AnyElement>& args,
                         std::string& peer_id, std::string& lobby_id, std::string& game_id,
                         bool& has_error);
    boost::container::flat_set<std::string> open();
    void close();
};
