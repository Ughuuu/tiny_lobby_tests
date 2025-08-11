#include "script_lua.h"

#include <filesystem>
#include <variant>

#include "../game/game_thread.h"
#include "luacode.h"
#include "lualib.h"
#include "script_lua_functions.h"

void create_lobby_userdata(lua_State* L, GameThread* game_thread, const std::string& game_id,
                           const std::string& lobby_id, const std::string& calling_peer_id,
                           const std::string& peer_id, LobbyUserdata::LobbyType type) {
    auto* ud = static_cast<LobbyUserdata*>(lua_newuserdata(L, sizeof(LobbyUserdata)));
    new (ud) LobbyUserdata{type, game_thread, game_id, lobby_id, calling_peer_id, peer_id};

    luaL_getmetatable(L, "LobbyMetatable");
    lua_setmetatable(L, -2);
}

static int lobby_newindex(lua_State* L) {
    LobbyUserdata* info = static_cast<LobbyUserdata*>(lua_touserdata(L, 1));
    if (!info) {
        luaL_error(L, "Expected light userdata as first argument.");
        return 0;
    }
    std::string calling_peer_id = info->calling_peer_id;
    std::string game_id = info->game_id;
    std::string lobby_id = info->lobby_id;
    GameThread* game_thread = info->game_thread;
    auto& game = game_thread->games[game_id];
    auto& lobby = game.lobbies[lobby_id];
    const char* key = luaL_checkstring(L, 2);

    switch (info->type) {
        case LobbyUserdata::LobbyType::LOBBY_ROOT: {
            if (strcmp(key, "sealed") == 0) {
                bool new_sealed = lua_toboolean(L, 3);
                if (new_sealed != lobby.sealed) {
                    lobby.sealed = new_sealed;
                    lobby.sealed_dirty = true;
                }
            } else if (strcmp(key, "name") == 0) {
                const char* new_name = lua_tostring(L, 3);
                if (new_name != lobby.name) {
                    lobby.name = new_name;
                    lobby.name_dirty = true;
                }
            } else if (strcmp(key, "host") == 0) {
                const char* new_host = lua_tostring(L, 3);
                if (new_host != lobby.host) {
                    game_thread->set_lobby_host(game, lobby, new_host);
                }
            } else if (strcmp(key, "max_players") == 0) {
                int new_max_players = lua_tointegerx(L, 3, nullptr);
                if (new_max_players != lobby.max_players) {
                    lobby.max_players = new_max_players;
                    lobby.max_players_dirty = true;
                }
            } else if (strcmp(key, "public_data") == 0) {
                AnyElement new_public_data = decode_luavalue(L, 3);
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        new_public_data.value)) {
                    auto& new_public_data_dict =
                        std::get<boost::container::flat_map<std::string, AnyElement>>(
                            new_public_data.value);
                    if (lobby.public_data_dirty || lobby.public_data != new_public_data_dict) {
                        lobby.public_data = new_public_data_dict;
                        lobby.public_data_diff = new_public_data_dict;
                        lobby.public_data_dirty = true;
                    }
                }
            } else if (strcmp(key, "private_data") == 0) {
                AnyElement new_private_data = decode_luavalue(L, 3);
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        new_private_data.value)) {
                    auto& new_private_data_dict =
                        std::get<boost::container::flat_map<std::string, AnyElement>>(
                            new_private_data.value);
                    if (lobby.private_data_dirty || lobby.private_data != new_private_data_dict) {
                        lobby.private_data = new_private_data_dict;
                        lobby.private_data_diff = new_private_data_dict;
                        lobby.private_data_dirty = true;
                    }
                }
            } else if (strcmp(key, "tags") == 0) {
                AnyElement new_tags = decode_luavalue(L, 3);
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        new_tags.value)) {
                    auto& new_tags_dict =
                        std::get<boost::container::flat_map<std::string, AnyElement>>(
                            new_tags.value);
                    if (lobby.tags_dirty || lobby.tags != new_tags_dict) {
                        lobby.tags = new_tags_dict;
                        lobby.tags_diff = new_tags_dict;
                        lobby.tags_dirty = true;
                    }
                }
            }
        } break;
        case LobbyUserdata::LobbyType::LOBBY_TAGS: {
            auto new_tag = decode_luavalue(L, 3);
            lobby.tags[key] = new_tag;
            lobby.tags_dirty = true;
            lobby.tags_diff[key] = new_tag;
        } break;
        case LobbyUserdata::LobbyType::LOBBY_PUBLIC_DATA: {
            auto new_public_data = decode_luavalue(L, 3);
            lobby.public_data[key] = new_public_data;
            lobby.public_data_dirty = true;
            lobby.public_data_diff[key] = new_public_data;
        } break;
        case LobbyUserdata::LobbyType::LOBBY_PRIVATE_DATA: {
            auto new_private_data = decode_luavalue(L, 3);
            lobby.private_data[key] = new_private_data;
            lobby.private_data_dirty = true;
            lobby.private_data_diff[key] = new_private_data;
        } break;
        case LobbyUserdata::LobbyType::PEER_ROOT: {
            auto& peer = game.peers[info->peer_id];
            if (strcmp(key, "public_data") == 0) {
                AnyElement new_public_data = decode_luavalue(L, 3);
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        new_public_data.value)) {
                    auto& new_public_data_dict =
                        std::get<boost::container::flat_map<std::string, AnyElement>>(
                            new_public_data.value);
                    if (peer.public_data_dirty || peer.public_data != new_public_data_dict) {
                        peer.public_data = new_public_data_dict;
                        peer.public_data_dirty = true;
                        peer.public_data_diff = new_public_data_dict;
                    }
                }
            } else if (strcmp(key, "private_data") == 0) {
                AnyElement new_private_data = decode_luavalue(L, 3);
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        new_private_data.value)) {
                    auto& new_private_data_dict =
                        std::get<boost::container::flat_map<std::string, AnyElement>>(
                            new_private_data.value);
                    if (peer.private_data_dirty || peer.private_data != new_private_data_dict) {
                        peer.private_data = new_private_data_dict;
                        peer.private_data_dirty = true;
                        peer.private_data_diff = new_private_data_dict;
                    }
                }
            }
        } break;
        case LobbyUserdata::LobbyType::PEER_PUBLIC_DATA: {
            auto& peer = game.peers[info->peer_id];
            auto new_public_data = decode_luavalue(L, 3);
            peer.public_data[key] = new_public_data;
            peer.public_data_dirty = true;
            peer.public_data_diff[key] = new_public_data;
        } break;
        case LobbyUserdata::LobbyType::PEER_PRIVATE_DATA: {
            auto& peer = game.peers[info->peer_id];
            auto new_private_data = decode_luavalue(L, 3);
            peer.private_data[key] = new_private_data;
            peer.private_data_dirty = true;
            peer.private_data_diff[key] = new_private_data;
        } break;
        case LobbyUserdata::LobbyType::PEER_USER_DATA: {
            auto& peer = game.peers[info->peer_id];
            auto new_user_data = decode_luavalue(L, 3);
            peer.user_data[key] = new_user_data;
            peer.user_data_dirty = true;
            peer.user_data_diff[key] = new_user_data;
        } break;
    }

    return 0;
}

// Returns cache table at top of stack, creates if missing
void get_lobby_userdata_cache(lua_State* L, int userdata_index) {
    lua_getfield(L, LUA_REGISTRYINDEX, "lobby_userdata_cache");  // cache
    lua_pushvalue(L, userdata_index);                            // push userdata
    lua_gettable(L, -2);                                         // cache[userdata]
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);                     // pop nil
        lua_newtable(L);                   // new cache table
        lua_pushvalue(L, userdata_index);  // push userdata
        lua_pushvalue(L, -2);              // push new cache table
        lua_settable(L, -4);               // cache[userdata] = new cache table
    }
    lua_remove(L, -2);  // remove cache table, leave cache table for this userdata
}

// Helper to cache a property in the userdata's registry weak table cache.
// - L: Lua state
// - key: property name (from Lua stack at index 2)
// - push_value: lambda that pushes the value to cache if not present
// Returns: true if value was found in cache and pushed, false if you should compute and cache it.
template <typename PushValueFunc>
bool cache_lobby_property(lua_State* L, const char* key, PushValueFunc push_value) {
    // stack: userdata, key
    get_lobby_userdata_cache(L, 1);  // stack: userdata, key, cache (for this userdata)
    lua_pushstring(L, key);          // push key
    lua_gettable(L, -2);             // cache[key]
    if (!lua_isnil(L, -1)) {
        // Found in cache
        lua_remove(L, -2);  // remove cache, leave value
        return true;
    }
    lua_pop(L, 1);  // pop nil

    // Not in cache, compute and cache it
    push_value();            // should push value to stack
    lua_pushstring(L, key);  // key
    lua_pushvalue(L, -2);    // value
    lua_settable(L, -4);     // cache[key] = value
    lua_remove(L, -2);       // remove cache, leave value
    return false;
}

static int lobby_index(lua_State* L) {
    LobbyUserdata* info = static_cast<LobbyUserdata*>(lua_touserdata(L, 1));
    if (!info) {
        luaL_error(L, "Expected light userdata as first argument.");
        return 0;
    }
    std::string game_id = info->game_id;
    std::string lobby_id = info->lobby_id;
    std::string calling_peer_id = info->calling_peer_id;
    GameThread* game_thread = info->game_thread;
    auto& game = game_thread->games[game_id];
    auto& lobby = game.lobbies[lobby_id];
    const char* key = luaL_checkstring(L, 2);
    std::string empty_string;
    const char* method = nullptr;

    // functions
    if (strcmp(key, "start_timer") == 0) {
        lua_pushcfunction(L, start_timer, "start_timer");
        return 1;
    } else if (strcmp(key, "stop_timer") == 0) {
        lua_pushcfunction(L, stop_timer, "stop_timer");
        return 1;
    } else if (strcmp(key, "notify") == 0) {
        lua_pushcfunction(L, notify, "notify");
        return 1;
    } else if (strcmp(key, "notify_all") == 0) {
        lua_pushcfunction(L, notify_all, "notify_all");
        return 1;
    } else if (strcmp(key, "kick_peer") == 0) {
        lua_pushcfunction(L, kick_peer, "kick_peer");
        return 1;
    } else if (strcmp(key, "set_leaderboard") == 0) {
        lua_pushcfunction(L, set_leaderboard, "set_leaderboard");
        return 1;
    } else if (strcmp(key, "broadcast_chat") == 0) {
        lua_pushcfunction(L, broadcast_chat, "broadcast_chat");
        return 1;
    }

    // properties
    switch (info->type) {
        case LobbyUserdata::LobbyType::LOBBY_ROOT: {
            if (strcmp(key, "calling_peer_id") == 0) {
                lua_pushstring(L, calling_peer_id.c_str());
                return 1;
            } else if (strcmp(key, "id") == 0) {
                lua_pushstring(L, lobby_id.c_str());
                return 1;
            } else if (strcmp(key, "tick_rate") == 0) {
                lua_pushnumber(L, game.tick_rate);
                return 1;
            } else if (strcmp(key, "name") == 0) {
                lua_pushstring(L, lobby.name.c_str());
                return 1;
            } else if (strcmp(key, "host") == 0) {
                lua_pushstring(L, lobby.host.c_str());
                return 1;
            } else if (strcmp(key, "max_players") == 0) {
                lua_pushnumber(L, lobby.max_players);
                return 1;
            } else if (strcmp(key, "create_time") == 0) {
                lua_pushnumber(L, int64_t(lobby.create_time));
                return 1;
            } else if (strcmp(key, "sealed") == 0) {
                lua_pushboolean(L, lobby.sealed);
                return 1;
            } else if (strcmp(key, "tags") == 0) {
                if (cache_lobby_property(L, key, [&]() {
                        create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id,
                                              empty_string, LobbyUserdata::LobbyType::LOBBY_TAGS);
                    })) {
                    return 1;
                }
                return 1;
            } else if (strcmp(key, "public_data") == 0) {
                if (cache_lobby_property(L, key, [&]() {
                        create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id,
                                              empty_string,
                                              LobbyUserdata::LobbyType::LOBBY_PUBLIC_DATA);
                    })) {
                    return 1;
                }
                return 1;
            } else if (strcmp(key, "private_data") == 0) {
                if (cache_lobby_property(L, key, [&]() {
                        create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id,
                                              empty_string,
                                              LobbyUserdata::LobbyType::LOBBY_PRIVATE_DATA);
                    })) {
                    return 1;
                }
                return 1;
            } else if (strcmp(key, "peers") == 0) {
                if (cache_lobby_property(L, key, [&]() {
                        lua_newtable(L);

                        for (const auto& peer_id : lobby.peer_ids) {
                            auto& peer = game.peers[peer_id];
                            create_lobby_userdata(L, game_thread, game_id, lobby_id,
                                                  calling_peer_id, peer_id,
                                                  LobbyUserdata::LobbyType::PEER_ROOT);
                            lua_setfield(L, -2, peer_id.c_str());
                        }
                    })) {
                    return 1;
                }
                return 1;
            } else if (strcmp(key, "peers_count") == 0) {
                lua_pushinteger(L, lobby.peer_ids.size());
                return 1;
            } else if (strcmp(key, "peers_ordered") == 0) {
                if (cache_lobby_property(L, key, [&]() {
                        lua_newtable(L);
                        int index = 1;
                        for (const auto& peer_id : lobby.peer_ordered_ids) {
                            auto& peer = game.peers[peer_id];
                            create_lobby_userdata(L, game_thread, game_id, lobby_id,
                                                  calling_peer_id, peer_id,
                                                  LobbyUserdata::LobbyType::PEER_ROOT);
                            lua_pushinteger(L, index++);  // push index
                            lua_insert(L, -2);            // move userdata below index
                            lua_settable(L, -3);          // table[index] = userdata
                        }
                    })) {
                    return 1;
                }
                return 1;
            }
        } break;
        case LobbyUserdata::LobbyType::LOBBY_TAGS: {
            if (lobby.tags.find(key) != lobby.tags.end()) {
                push_lua_value(L, lobby.tags[key]);
                return 1;
            }
        } break;
        case LobbyUserdata::LobbyType::LOBBY_PUBLIC_DATA: {
            if (lobby.public_data.find(key) != lobby.public_data.end()) {
                push_lua_value(L, lobby.public_data[key]);
                return 1;
            }
        } break;
        case LobbyUserdata::LobbyType::LOBBY_PRIVATE_DATA: {
            if (lobby.private_data.find(key) != lobby.private_data.end()) {
                push_lua_value(L, lobby.private_data[key]);
                return 1;
            }
        } break;
        case LobbyUserdata::LobbyType::PEER_ROOT: {
            auto& peer = game.peers[info->peer_id];
            if (strcmp(key, "id") == 0) {
                lua_pushstring(L, peer.id.c_str());
                return 1;
            } else if (strcmp(key, "order_id") == 0) {
                lua_pushinteger(L, peer.order_id);
                return 1;
            } else if (strcmp(key, "platform") == 0) {
                lua_pushstring(L, peer.platform.c_str());
                return 1;
            } else if (strcmp(key, "platform_id") == 0) {
                lua_pushstring(L, peer.platform_id.c_str());
                return 1;
            } else if (strcmp(key, "ready") == 0) {
                lua_pushboolean(L, peer.ready);
                return 1;
            } else if (strcmp(key, "disconnected") == 0) {
                lua_pushboolean(L, peer.disconnected);
                return 1;
            } else if (strcmp(key, "public_data") == 0) {
                if (cache_lobby_property(L, key, [&]() {
                        create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id,
                                              peer.id, LobbyUserdata::LobbyType::PEER_PUBLIC_DATA);
                    })) {
                    return 1;
                }
                return 1;
            } else if (strcmp(key, "private_data") == 0) {
                if (cache_lobby_property(L, key, [&]() {
                        create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id,
                                              peer.id, LobbyUserdata::LobbyType::PEER_PRIVATE_DATA);
                    })) {
                    return 1;
                }
                return 1;
            } else if (strcmp(key, "user_data") == 0) {
                if (cache_lobby_property(L, key, [&]() {
                        create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id,
                                              peer.id, LobbyUserdata::LobbyType::PEER_USER_DATA);
                    })) {
                    return 1;
                }
                return 1;
            }
        } break;
        case LobbyUserdata::LobbyType::PEER_PUBLIC_DATA: {
            auto& peer = game.peers[info->peer_id];
            if (peer.public_data.find(key) != peer.public_data.end()) {
                push_lua_value(L, peer.public_data[key]);
                return 1;
            }
        } break;
        case LobbyUserdata::LobbyType::PEER_PRIVATE_DATA: {
            auto& peer = game.peers[info->peer_id];
            if (peer.private_data.find(key) != peer.private_data.end()) {
                push_lua_value(L, peer.private_data[key]);
                return 1;
            }
        } break;
        case LobbyUserdata::LobbyType::PEER_USER_DATA: {
            auto& peer = game.peers[info->peer_id];
            if (peer.user_data.find(key) != peer.user_data.end()) {
                push_lua_value(L, peer.user_data[key]);
                return 1;
            }
        } break;
    }
    return 0;
}

void ScriptLua::set_lua_metatables() {
    // set cache
    lua_newtable(L);  // cache table
    lua_newtable(L);  // metatable
    lua_pushstring(L, "v");
    lua_setfield(L, -2, "__mode");  // weak values
    lua_setmetatable(L, -2);
    lua_setfield(L, LUA_REGISTRYINDEX, "lobby_userdata_cache");
    // game thread
    lua_pushlightuserdata(L, game_thread);
    lua_setfield(L, LUA_REGISTRYINDEX, "game_thread");

    // lobby metatable
    luaL_newmetatable(L, "LobbyMetatable");

    lua_pushstring(L, "__index");

    lua_pushcfunction(L, lobby_index, "lobby_index");
    lua_settable(L, -3);
    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, lobby_newindex, "lobby_newindex");
    lua_settable(L, -3);
    lua_pop(L, 1);
    luaopen_lobby(L);
    luaopen_system(L);
}

boost::container::flat_set<std::string> ScriptLua::open() {
    if (enabled) {
        close();
    }
    boost::container::flat_set<std::string> empty_set;
    // do not open if folder_name is empty
    if (folder_name.empty()) {
        return empty_set;
    }
    L = luaL_newstate();
    std::string base_path = scripts_folder + "/" + folder_name;
    lua_pushstring(L, base_path.c_str());
    lua_setfield(L, LUA_REGISTRYINDEX, "base_path");

    if (!L) {
        enabled = false;
        return empty_set;
    }
    setup_lua_state(L);
    set_lua_metatables();
    enabled = true;
    return run_lua_file(L, (base_path + "/" + script_entrypoint).c_str());
}

AnyElement ScriptLua::func_call(std::string& func_name, boost::container::vector<AnyElement>& args,
                                std::string& peer_id, std::string& lobby_id, std::string& game_id,
                                bool& has_error) {
    if (!enabled) {
        return AnyElement{"Lua script is not enabled"};
    }
    if (!enabled) {
        return AnyElement{"Lua script is not enabled"};
    }
    if (L == nullptr) {
        return AnyElement{"Lua state is nullptr"};
    }

    lua_pushstring(L, peer_id.c_str());
    lua_setfield(L, LUA_REGISTRYINDEX, "peer_id");

    lua_pushstring(L, lobby_id.c_str());
    lua_setfield(L, LUA_REGISTRYINDEX, "lobby_id");

    lua_pushstring(L, game_id.c_str());
    lua_setfield(L, LUA_REGISTRYINDEX, "game_id");

    lua_getfield(L, LUA_REGISTRYINDEX, "main");
    if (!lua_istable(L, -1)) {
        return AnyElement{"Main is not a table. "};
    }
    int ret = lua_getfield(L, -1, func_name.c_str());
    if (ret != LUA_TFUNCTION) {
        has_error = true;
        lua_settop(L, 0);
        return AnyElement{"Function not found. " + func_name};
    }

    // Push arguments onto the Lua stack
    for (int i = 0; i < args.size(); i++) {
        push_lua_value(L, args[i]);
    }
    int status = lua_pcall(L, args.size(), LUA_MULTRET, 0);
    if (status != 0) {
        has_error = true;
        const char* msg = lua_tostring(L, -1);
        std::string error_msg = msg ? msg : "Unknown Lua error";
        lua_settop(L, 0);
        return AnyElement{error_msg};
    }
    int lua_top = lua_gettop(L);
    if (lua_top > 0) {
        auto lua_result = decode_luavalue(L, -1);
        lua_settop(L, 0);
        return lua_result;
    }
    lua_settop(L, 0);
    return AnyElement{std::monostate{}};
}

void ScriptLua::close() {
    if (L) {
        lua_close(L);
        L = nullptr;
    }
}
