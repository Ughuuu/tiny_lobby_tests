#pragma once
extern "C" {
    #include "lua.h"
    #include "lauxlib.h"
    #include "lualib.h"
}
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "INIReader.h"
#include "server_logger.h"
#include "any_type.h"

class GameThread;

struct ScriptLua {
    lua_State *L = nullptr;
    bool autoreload;
    std::string script_language;
    std::string scripts_folder;
    std::string folder_name;
    std::string script_entrypoint;
    std::string logs_folder;
    GameThread *game_thread;
public:
    bool enabled = false;
    AnyElement func_call(std::string &func_name, std::vector<AnyElement> &args, std::string &lobby_id, std::string &game_id, bool &has_error);
    void open();
    void close();
};
