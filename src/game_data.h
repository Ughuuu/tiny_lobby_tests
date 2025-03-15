#pragma once
#include <string>
#include "peer_data.h"
#include "lobby_data.h"
#include "lua.hpp"
#include "angelscript.h"

struct LuaGameData {
    lua_State *L = nullptr;
};

struct AngelScriptGameData {
    asIScriptEngine *engine = nullptr;
    asIScriptModule *mod = nullptr;
    asIScriptFunction *func = nullptr;
};

struct GameData {
    std::string id;
    std::string entrypoint;
    long tick_rate = 0;
    std::unordered_map<std::string, PeerData> peers;
    std::unordered_map<std::string, LobbyData> lobbies;
    LuaGameData lua;
    AngelScriptGameData angelscript;
};
