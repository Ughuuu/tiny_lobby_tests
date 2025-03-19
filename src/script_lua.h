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

struct LuaWrapperInfo {
    std::string name;
    int idx = 0;
};

struct ScriptLua {
    lua_State *L;
    bool autoreload;
    std::string script_language;
    std::string scripts_folder;
    std::string folder_name;
    std::string script_entrypoint;
    std::string logs_folder;
    GameThread *game_thread;
    LuaWrapperInfo lobby_wrapper = { .name = "lobby" };
    LuaWrapperInfo tags_wrapper = { .name = "tags" };
    LuaWrapperInfo public_data_wrapper = { .name = "public_data" };
    LuaWrapperInfo private_data_wrapper = { .name = "private_data" };
    // peers array of 10
    LuaWrapperInfo peers_elements_wrapper[10] = {
        { .name = "peer_object", .idx = 0 },
        { .name = "peer_object", .idx = 1 },
        { .name = "peer_object", .idx = 2 },
        { .name = "peer_object", .idx = 3 },
        { .name = "peer_object", .idx = 4 },
        { .name = "peer_object", .idx = 5 },
        { .name = "peer_object", .idx = 6 },
        { .name = "peer_object", .idx = 7 },
        { .name = "peer_object", .idx = 8 },
        { .name = "peer_object", .idx = 9 },
    };
    LuaWrapperInfo peers_public_wrapper[10] = {
        { .name = "peer_public_data", .idx = 0 },
        { .name = "peer_public_data", .idx = 1 },
        { .name = "peer_public_data", .idx = 2 },
        { .name = "peer_public_data", .idx = 3 },
        { .name = "peer_public_data", .idx = 4 },
        { .name = "peer_public_data", .idx = 5 },
        { .name = "peer_public_data", .idx = 6 },
        { .name = "peer_public_data", .idx = 7 },
        { .name = "peer_public_data", .idx = 8 },
        { .name = "peer_public_data", .idx = 9 },
    };
    LuaWrapperInfo peers_private_wrapper[10] = {
        { .name = "peer_private_data", .idx = 0 },
        { .name = "peer_private_data", .idx = 1 },
        { .name = "peer_private_data", .idx = 2 },
        { .name = "peer_private_data", .idx = 3 },
        { .name = "peer_private_data", .idx = 4 },
        { .name = "peer_private_data", .idx = 5 },
        { .name = "peer_private_data", .idx = 6 },
        { .name = "peer_private_data", .idx = 7 },
        { .name = "peer_private_data", .idx = 8 },
        { .name = "peer_private_data", .idx = 9 },
    };
    LuaWrapperInfo peers_user_wrapper[10] = {
        { .name = "peer_user_data", .idx = 0 },
        { .name = "peer_user_data", .idx = 1 },
        { .name = "peer_user_data", .idx = 2 },
        { .name = "peer_user_data", .idx = 3 },
        { .name = "peer_user_data", .idx = 4 },
        { .name = "peer_user_data", .idx = 5 },
        { .name = "peer_user_data", .idx = 6 },
        { .name = "peer_user_data", .idx = 7 },
        { .name = "peer_user_data", .idx = 8 },
        { .name = "peer_user_data", .idx = 9 },
    };
public:
    bool enabled = false;
    AnyElement func_call(std::string &func_name, std::vector<AnyElement> &args, std::string &lobby_id, std::string &game_id, bool &has_error);
    void open();
    void close();
    void set_lua_metatables();
};
