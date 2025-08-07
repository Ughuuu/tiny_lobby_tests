#pragma once
#include "../common/any_type.h"
#include "lua.h"

AnyElement decode_luavalue(lua_State *L, int idx);
AnyElement decode_luatable(lua_State *L, int idx);
void push_lua_value(lua_State *L, const AnyElement &value);
void push_table(lua_State *L, const boost::container::vector<AnyElement> &array);
void push_table(lua_State *L, const boost::container::flat_map<std::string, AnyElement> &table);
void push_lua_value(lua_State *L, const AnyElement &value);
int start_timer(lua_State *L);
int stop_timer(lua_State *L);
int notify(lua_State *L);
int notify_all(lua_State *L);
int broadcast_chat(lua_State *L);
int get_time(lua_State *L);
int get_lobby(lua_State *L);
int kick_peer(lua_State *L);
int set_leaderboard(lua_State *L);

int lua_require(lua_State *L);
void setup_lua_state(lua_State *L);
boost::container::flat_set<std::string> run_lua_file(lua_State *L, std::string name);
void luaopen_lobby(lua_State *L);
void luaopen_system(lua_State *L);
