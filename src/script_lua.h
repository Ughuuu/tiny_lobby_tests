#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "INIReader.h"
#include "any_type.h"
#include "lua.h"
#include "server_logger.h"

class GameThread;

struct LuaWrapperInfo {
    std::string name;
    int idx = -1;
};

struct ScriptLua {
    lua_State *L;
    bool autoreload;
    std::string scripts_folder;
    std::string folder_name;
    std::string script_entrypoint;
    std::string logs_folder;
    GameThread *game_thread;
    LuaWrapperInfo lobby_wrapper = {.name = "lobby", .idx = 0};
    LuaWrapperInfo tags_wrapper = {.name = "tags", .idx = 0};
    LuaWrapperInfo public_data_wrapper = {.name = "public_data", .idx = 0};
    LuaWrapperInfo private_data_wrapper = {.name = "private_data", .idx = 0};

   public:
    bool enabled = false;
    AnyElement func_call(std::string &func_name, boost::container::vector<AnyElement> &args,
                         std::string &peer_id, std::string &lobby_id, std::string &game_id,
                         bool &has_error);
    boost::container::flat_set<std::string> open();
    void close();
    void set_lua_metatables();
};
