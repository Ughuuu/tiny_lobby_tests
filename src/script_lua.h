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

struct LobbyUserDataKey {
    std::string key;
    int idx = -1;
};

struct LobbyUserdata {
    enum LobbyType {
        LOBBY_ROOT,
        LOBBY_PUBLIC_DATA,
        LOBBY_PRIVATE_DATA,
        LOBBY_TAGS,
        PEER_ROOT,
        PEER_PUBLIC_DATA,
        PEER_PRIVATE_DATA,
        PEER_USER_DATA,
        NESTED_MAP
    } type;

    GameThread* game_thread;
    std::string game_id;
    std::string lobby_id;
    std::string calling_peer_id;
    std::string peer_id;
    boost::container::vector<LobbyUserDataKey> keys;
};

void create_lobby_userdata(lua_State* L, GameThread* game_thread, const std::string& game_id,
                           const std::string& lobby_id, const std::string& calling_peer_id,
                           const std::string& peer_id,
                           const boost::container::vector<LobbyUserDataKey>& keys,
                           LobbyUserdata::LobbyType type);

struct ScriptLua {
    lua_State* L;
    bool autoreload;
    std::string scripts_folder;
    std::string folder_name;
    std::string script_entrypoint;
    std::string logs_folder;
    GameThread* game_thread;

   public:
    bool enabled = false;
    AnyElement func_call(std::string& func_name, boost::container::vector<AnyElement>& args,
                         std::string& peer_id, std::string& lobby_id, std::string& game_id,
                         bool& has_error);
    boost::container::flat_set<std::string> open();
    void close();
    void set_lua_metatables();
};
