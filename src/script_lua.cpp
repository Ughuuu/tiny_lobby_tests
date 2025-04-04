#include "script_lua.h"

#include <filesystem>
#include <variant>

#include "game_thread.h"
#include "luacode.h"
#include "lualib.h"

static AnyElement decode_luavalue(lua_State *L, int idx);

static AnyElement decode_luatable(lua_State *L, int idx) {
    boost::container::flat_map<std::string, AnyElement> result_dict;
    boost::container::vector<AnyElement> result_array;
    if (!lua_istable(L, idx)) {
        return AnyElement{std::monostate{}};
    }
    lua_pushnil(L);
    bool is_dict = true;
    while (lua_next(L, idx) != 0) {
        if (lua_isnumber(L, -2)) {
            auto key = lua_tointeger(L, -2);
            result_array.push_back(decode_luavalue(L, -1));
            is_dict = false;
        } else if (lua_isstring(L, -2)) {
            lua_pushvalue(L, -2);
            std::string key = lua_tostring(L, -1);
            if (key.size() == 0) {
                luaL_error(L, "empty key");
                return AnyElement{std::monostate{}};
            }
            if (key[0] >= '0' && key[0] <= '9' && !is_dict) {
                is_dict = false;
                int num = std::stoi(key);
                if (num >= 1) {
                    result_array.push_back(decode_luavalue(L, -1));
                }
            } else {
                auto value_decoded = decode_luavalue(L, -2);
                // do not put nil values in dictionary
                if (!std::holds_alternative<std::monostate>(value_decoded.value)) {
                    result_dict.emplace(key, value_decoded);
                }
            }
            lua_pop(L, 1);
        } else {
            luaL_error(L, "invalid key type");
            return AnyElement{std::monostate{}};
        }
        lua_pop(L, 1);
    }
    if (is_dict) {
        return AnyElement{result_dict};
    } else {
        return AnyElement{result_array};
    }
}

static AnyElement decode_luavalue(lua_State *L, int idx) {
    switch (lua_type(L, idx)) {
        case LUA_TNIL:
            return AnyElement{std::monostate{}};
        case LUA_TBOOLEAN:
            return AnyElement{bool(lua_toboolean(L, idx))};
        case LUA_TNUMBER: {
            double n = lua_tonumber(L, idx);
            if (n == (int64_t)n) {
                return AnyElement{(int64_t)n};
            }
            return AnyElement{n};
        }
        case LUA_TSTRING:
            return AnyElement{lua_tostring(L, idx)};
        case LUA_TTABLE:
            return decode_luatable(L, lua_gettop(L));
        default:
            return AnyElement{std::monostate{}};
    }
}

static void push_lua_value(lua_State *L, const AnyElement &value);

static void push_table(lua_State *L, const boost::container::vector<AnyElement> &array) {
    lua_createtable(L, array.size(), 0);
    for (size_t i = 0; i < array.size(); ++i) {
        lua_pushinteger(L, i + 1);
        push_lua_value(L, array[i]);
        lua_settable(L, -3);
    }
}

static void push_table(lua_State *L,
                       const boost::container::flat_map<std::string, AnyElement> &table) {
    lua_createtable(L, table.size(), 0);
    for (const auto &pair : table) {
        lua_pushstring(L, pair.first.c_str());
        push_lua_value(L, pair.second);
        lua_settable(L, -3);
    }
}

static void push_lua_value(lua_State *L, const AnyElement &value) {
    std::visit(
        [L](auto &&v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                lua_pushnil(L);
            } else if constexpr (std::is_same_v<T, bool>) {
                lua_pushboolean(L, v);
            } else if constexpr (std::is_same_v<T, int64_t>) {
                lua_pushinteger(L, v);
            } else if constexpr (std::is_same_v<T, double>) {
                lua_pushnumber(L, v);
            } else if constexpr (std::is_same_v<T, std::string>) {
                lua_pushlstring(L, v.c_str(), v.size());
            } else if constexpr (std::is_same_v<T, boost::container::vector<AnyElement>>) {
                push_table(L, v);
            } else if constexpr (std::is_same_v<
                                     T, boost::container::flat_map<std::string, AnyElement>>) {
                push_table(L, v);
            } else {
                lua_pushnil(L);
            }
        },
        value.value);
}

static int start_timer(lua_State *L) {
    if (lua_gettop(L) < 2) {
        luaL_error(L, "Expected at least 2 arguments.");
        return 0;
    }
    std::string timer_id = luaL_checkstring(L, 1);
    int duration = luaL_checkinteger(L, 2);
    if (duration < 1 || duration > 300) {
        luaL_error(L, "Timer duration must be between 1 second and 5 minutes.");
        return 0;
    }
    boost::container::vector<AnyElement> args;
    // Get the additional parameters sent with the timer
    for (int i = 3; i <= lua_gettop(L); ++i) {
        args.push_back(decode_luavalue(L, i));
    }

    lua_getfield(L, LUA_REGISTRYINDEX, "peer_id");
    std::string peer_id = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "game_id");
    std::string game_id = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "lobby_id");
    std::string lobby_id = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "game_thread");
    GameThread *game_thread = static_cast<GameThread *>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    auto &game = game_thread->games[game_id];
    game.timer_data.insert_or_assign(timer_id,
                                     TimerData{
                                         .id = timer_id,
                                         .lobby_id = lobby_id,
                                         .game_id = game_id,
                                         .peer_id = peer_id,
                                         .end_time = duration * 1000 + game_thread->get_time(),
                                         .args = args,
                                     });

    return 0;
}

static int stop_timer(lua_State *L) {
    if (lua_gettop(L) < 1) {
        luaL_error(L, "Expected 1 argument.");
        return 0;
    }
    std::string timer_id = luaL_checkstring(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, "game_id");
    std::string game_id = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "lobby_id");
    std::string lobby_id = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "game_thread");
    GameThread *game_thread = static_cast<GameThread *>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    auto &game = game_thread->games[game_id];
    game.timer_data.erase(timer_id);
    return 0;
}

static int notify(lua_State *L) {
    if (lua_gettop(L) < 2) {
        luaL_error(L, "Expected 2 arguments.");
        return 0;
    }
    const char *peer_id = luaL_checkstring(L, 1);
    auto notification_object = decode_luavalue(L, 2);
    lua_getfield(L, LUA_REGISTRYINDEX, "game_id");
    std::string game_id = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "lobby_id");
    std::string lobby_id = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "game_thread");
    GameThread *game_thread = static_cast<GameThread *>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    auto &game = game_thread->games[game_id];
    game_thread->notify_peer(game, lobby_id, peer_id, notification_object.to_string());
    return 0;
}

static int broadcast_chat(lua_State *L) {
    if (lua_gettop(L) < 1) {
        luaL_error(L, "Expected 1 arguments.");
        return 0;
    }
    std::string message = luaL_checkstring(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, "game_id");
    std::string game_id = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "lobby_id");
    std::string lobby_id = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "game_thread");
    GameThread *game_thread = static_cast<GameThread *>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    auto &game = game_thread->games[game_id];
    game_thread->send_message(game, lobby_id, message);
    return 0;
}

static int get_time(lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "game_thread");
    GameThread *game_thread = static_cast<GameThread *>(lua_touserdata(L, -1));
    lua_pushinteger(L, game_thread->get_time());
    return 1;
}

static int get_order_of_element(const std::set<std::string> &ordered_set, const std::string &key) {
    auto it = ordered_set.find(key);
    if (it != ordered_set.end()) {
        return std::distance(ordered_set.begin(), it);
    }
    return -1;
}

static std::string get_element_at_index(const boost::container::flat_set<std::string> &ordered_set,
                                        int index) {
    if (index < 0 || index >= ordered_set.size()) {
        return "";  // Invalid index
    }

    auto it = ordered_set.begin();
    std::advance(it, index);  // Move iterator to the specified index
    return *it;               // Return the element at the specified index
}

static int lobby_newindex(lua_State *L) {
    LuaWrapperInfo *info = static_cast<LuaWrapperInfo *>(lua_touserdata(L, 1));
    if (!info) {
        luaL_error(L, "Expected light userdata as first argument.");
        return 0;
    }
    lua_getfield(L, LUA_REGISTRYINDEX, "game_id");
    std::string game_id = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "lobby_id");
    std::string lobby_id = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "game_thread");
    GameThread *game_thread = static_cast<GameThread *>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    auto &game = game_thread->games[game_id];
    auto &lobby = game.lobbies[lobby_id];

    auto key = luaL_checkstring(L, 2);

    if (info->name == "lobby") {
        if (strcmp(key, "sealed") == 0) {
            bool new_sealed = lua_toboolean(L, 3);
            if (new_sealed != lobby.sealed) {
                lobby.sealed = new_sealed;
                lobby.sealed_dirty = true;
            }
        }
    } else if (info->name == "tags") {
        auto new_tag = decode_luavalue(L, 3);
        if (lobby.tags_dirty || new_tag != lobby.tags[key]) {
            lobby.tags[key] = new_tag;
            lobby.tags_dirty = true;
        }
    } else if (info->name == "public_data") {
        auto new_public_data = decode_luavalue(L, 3);
        if (lobby.public_data_dirty || new_public_data != lobby.public_data[key]) {
            lobby.public_data[key] = new_public_data;
            lobby.public_data_dirty = true;
        }
    } else if (info->name == "private_data") {
        auto new_private_data = decode_luavalue(L, 3);
        if (lobby.private_data_dirty || new_private_data != lobby.private_data[key]) {
            lobby.private_data[key] = new_private_data;
            lobby.private_data_dirty = true;
        }
    } else if (info->name == "peer_public_data") {
        auto peer_id = get_element_at_index(lobby.peer_ids, info->idx);
        auto &peer = game.peers[peer_id];
        auto new_public_data = decode_luavalue(L, 3);
        if (peer.public_data_dirty || new_public_data != peer.public_data[key]) {
            peer.public_data[key] = new_public_data;
            peer.public_data_dirty = true;
        }
    } else if (info->name == "peer_private_data") {
        auto peer_id = get_element_at_index(lobby.peer_ids, info->idx);
        auto &peer = game.peers[peer_id];
        auto new_private_data = decode_luavalue(L, 3);
        if (peer.private_data_dirty || new_private_data != peer.private_data[key]) {
            peer.private_data[key] = new_private_data;
            peer.private_data_dirty = true;
        }
    }

    return 0;
}

static int lobby_index(lua_State *L) {
    LuaWrapperInfo *info = static_cast<LuaWrapperInfo *>(lua_touserdata(L, 1));
    if (!info) {
        luaL_error(L, "Expected light userdata as first argument.");
        return 0;
    }

    lua_getfield(L, LUA_REGISTRYINDEX, "peer_id");
    std::string peer_id = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "game_id");
    std::string game_id = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "lobby_id");
    std::string lobby_id = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "game_thread");
    GameThread *game_thread = static_cast<GameThread *>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    auto &game = game_thread->games[game_id];
    auto &lobby = game.lobbies[lobby_id];

    const char *key = luaL_checkstring(L, 2);

    if (info->name == "lobby") {
        if (strcmp(key, "calling_peer_id") == 0) {
            lua_pushstring(L, peer_id.c_str());
            return 1;
        } else if (strcmp(key, "id") == 0) {
            lua_pushstring(L, lobby_id.c_str());
            return 1;
        } else if (strcmp(key, "name") == 0) {
            lua_pushstring(L, lobby.name.c_str());
            return 1;
        } else if (strcmp(key, "host") == 0) {
            lua_pushstring(L, lobby.host.c_str());
            return 1;
        } else if (strcmp(key, "max_players") == 0) {
            lua_pushinteger(L, lobby.max_players);
            return 1;
        } else if (strcmp(key, "create_time") == 0) {
            lua_pushinteger(L, int64_t(lobby.create_time));
            return 1;
        } else if (strcmp(key, "sealed") == 0) {
            lua_pushboolean(L, lobby.sealed);
            return 1;
        } else if (strcmp(key, "tags") == 0) {
            lua_getfield(L, LUA_REGISTRYINDEX, "tags_object");
            return 1;
        } else if (strcmp(key, "public_data") == 0) {
            lua_getfield(L, LUA_REGISTRYINDEX, "public_data_object");
            return 1;
        } else if (strcmp(key, "private_data") == 0) {
            lua_getfield(L, LUA_REGISTRYINDEX, "private_data_object");
            return 1;
        } else if (strcmp(key, "peers") == 0) {
            lua_newtable(L);

            int index = 0;
            for (const auto &peer_id : lobby.peer_ids) {
                auto &peer = game.peers[peer_id];
                lua_getfield(L, LUA_REGISTRYINDEX,
                             ("peer_object_" + std::to_string(index)).c_str());
                lua_setfield(L, -2, peer_id.c_str());
                index++;
            }
            return 1;
        } else if (strcmp(key, "peers_count") == 0) {
            lua_pushinteger(L, lobby.peer_ids.size());
            return 1;
        }
    } else if (info->name == "tags") {
        if (lobby.tags.find(key) != lobby.tags.end()) {
            push_lua_value(L, lobby.tags[key]);
            return 1;
        }
    } else if (info->name == "public_data") {
        if (lobby.public_data.find(key) != lobby.public_data.end()) {
            push_lua_value(L, lobby.public_data[key]);
            return 1;
        }
    } else if (info->name == "private_data") {
        if (lobby.private_data.find(key) != lobby.private_data.end()) {
            push_lua_value(L, lobby.private_data[key]);
            return 1;
        }
    } else if (info->name == "peer_object") {
        // peers
        auto peer_id = get_element_at_index(lobby.peer_ids, info->idx);
        auto &peer = game.peers[peer_id];
        if (strcmp(key, "id") == 0) {
            lua_pushstring(L, peer.id.c_str());
            return 1;
        } else if (strcmp(key, "order_id") == 0) {
            lua_pushinteger(L, peer.order_id);
            return 1;
        } else if (strcmp(key, "ready") == 0) {
            lua_pushboolean(L, peer.ready);
            return 1;
        } else if (strcmp(key, "disconnected") == 0) {
            lua_pushboolean(L, peer.disconnected);
            return 1;
        } else if (strcmp(key, "public_data") == 0) {
            lua_getfield(L, LUA_REGISTRYINDEX,
                         ("peer_public_data_" + std::to_string(info->idx)).c_str());
            return 1;
        } else if (strcmp(key, "private_data") == 0) {
            lua_getfield(L, LUA_REGISTRYINDEX,
                         ("peer_private_data_" + std::to_string(info->idx)).c_str());
            return 1;
        } else if (strcmp(key, "user_data") == 0) {
            lua_getfield(L, LUA_REGISTRYINDEX,
                         ("peer_user_data_" + std::to_string(info->idx)).c_str());
            return 1;
        }
    } else if (info->name == "peer_public_data") {
        auto peer_id = get_element_at_index(lobby.peer_ids, info->idx);
        auto &peer = game.peers[peer_id];
        if (peer.public_data.find(key) != peer.public_data.end()) {
            push_lua_value(L, peer.public_data[key]);
            return 1;
        }
    } else if (info->name == "peer_private_data") {
        auto peer_id = get_element_at_index(lobby.peer_ids, info->idx);
        auto &peer = game.peers[peer_id];
        if (peer.private_data.find(key) != peer.private_data.end()) {
            push_lua_value(L, peer.private_data[key]);
            return 1;
        }
    } else if (info->name == "peer_user_data") {
        auto peer_id = get_element_at_index(lobby.peer_ids, info->idx);
        auto &peer = game.peers[peer_id];
        if (peer.user_data.find(key) != peer.user_data.end()) {
            push_lua_value(L, peer.user_data[key]);
            return 1;
        }
    }

    return 0;
}

int get_lobby(lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "lobby_global");
    return 1;
}

static int lua_require(lua_State *L) {
    std::string modname = luaL_checkstring(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "base_path");
    std::string base_path = lua_tostring(L, -1);
    lua_pop(L, 1);

    if (std::string(modname).find("..") != std::string::npos) {
        lua_pushfstring(L, "require: cannot use ..");
        lua_error(L);
    }

    std::string resolvedPath = base_path + "/" + modname + ".lua";

    // Use absolute resolved path as the cache key
    lua_getfield(L, LUA_REGISTRYINDEX, "_MODULES");
    // lobby and system package
    if (modname == "lobby") {
        lua_getfield(L, -1, "lobby");
    } else if (modname == "system") {
        lua_getfield(L, -1, "system");
    } else {
        lua_getfield(L, -1, resolvedPath.c_str());
    }
    if (!lua_isnil(L, -1)) {
        return 1;  // Already cached
    }
    lua_pop(L, 1);  // Pop nil

    // Read the file contents
    std::ifstream file(resolvedPath);
    if (!file.is_open()) {
        lua_pushfstring(L, "require: cannot open file '%s'", resolvedPath.c_str());
        lua_error(L);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // Compile and load the module

    std::string chunkname = "@" + std::string(resolvedPath);
    size_t bytecodeSize = 0;
    char *bytecode = luau_compile(source.c_str(), source.size(), NULL, &bytecodeSize);

    if (luau_load(L, resolvedPath.c_str(), bytecode, bytecodeSize, 0) != 0) {
        const char *err = lua_tostring(L, -1);
        luaL_error(L, "require: failed to load module '%s': %s", resolvedPath.c_str(),
                   err ? err : "unknown error");
    }

    // Execute the module
    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        luaL_error(L, "require: failed to run module '%s': %s", resolvedPath.c_str(),
                   err ? err : "unknown error");
    }

    // Validate return type
    if (!lua_istable(L, -1) && !lua_isfunction(L, -1)) {
        lua_pushfstring(L, "require: module '%s' must return a table or function",
                        resolvedPath.c_str());
        lua_error(L);
    }

    // Store in _MODULES
    lua_pushvalue(L, -1);
    lua_setfield(L, -3, resolvedPath.c_str());

    return 1;
}

void setupState(lua_State *L) {
    luaL_openlibs(L);

    static const luaL_Reg funcs[] = {
        {"require", lua_require},
        {NULL, NULL},
    };

    lua_pushvalue(L, LUA_GLOBALSINDEX);
    luaL_register(L, NULL, funcs);
    lua_pop(L, 1);

    luaL_sandbox(L);
}

bool runFile(lua_State *L, std::string name) {
    std::ifstream file(name);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    std::string chunkname = "@" + std::string(name);
    size_t bytecodeSize = 0;
    char *bytecode = luau_compile(source.c_str(), source.size(), NULL, &bytecodeSize);
    int status = luau_load(L, chunkname.c_str(), bytecode, bytecodeSize, 0);
    free(bytecode);

    if (status != LUA_OK) {
        std::cerr << "Load error: " << lua_tostring(L, -1) << "\n";
        return false;
    } else {
        // Run the loaded chunk
        if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
            std::cerr << "Runtime error: " << lua_tostring(L, -1) << "\n";
            return false;
        }
        // Store the module in registry
        lua_setfield(L, LUA_REGISTRYINDEX, "main");
    }
    return true;
}

void luaopen_lobby(lua_State *L) {
    lua_newtable(L);

    lua_pushcfunction(L, get_lobby, "get_lobby");
    lua_setfield(L, -2, "get");

    lua_pushcfunction(L, start_timer, "start_timer");
    lua_setfield(L, -2, "start_timer");

    lua_pushcfunction(L, stop_timer, "stop_timer");
    lua_setfield(L, -2, "stop_timer");

    lua_pushcfunction(L, notify, "notify");
    lua_setfield(L, -2, "notify");

    lua_pushcfunction(L, broadcast_chat, "broadcast_chat");
    lua_setfield(L, -2, "broadcast_chat");

    luaL_findtable(L, LUA_REGISTRYINDEX, "_MODULES", 1);
    lua_pushstring(L, "lobby");
    lua_pushvalue(L, -3);
    lua_settable(L, -3);
    lua_pop(L, 2);
}

void luaopen_system(lua_State *L) {
    lua_newtable(L);

    lua_pushcfunction(L, get_time, "get_time");
    lua_setfield(L, -2, "get_time");

    luaL_findtable(L, LUA_REGISTRYINDEX, "_MODULES", 1);
    lua_pushstring(L, "system");
    lua_pushvalue(L, -3);
    lua_settable(L, -3);
    lua_pop(L, 2);
}

void ScriptLua::set_lua_metatables() {
    // game thread
    lua_pushlightuserdata(L, game_thread);
    lua_setfield(L, LUA_REGISTRYINDEX, "game_thread");

    // shared metatable
    luaL_newmetatable(L, "SharedMetatable");

    lua_pushstring(L, "__index");
    lua_pushcfunction(L, lobby_index, "lobby_index");
    lua_settable(L, -3);
    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, lobby_newindex, "lobby_newindex");
    lua_settable(L, -3);
    lua_pop(L, 1);

    auto register_object = [&](void *userdata, const char *name) {
        lua_pushlightuserdata(L, userdata);
        luaL_getmetatable(L, "SharedMetatable");
        lua_setmetatable(L, -2);
        lua_setfield(L, LUA_REGISTRYINDEX, name);
    };

    register_object(&tags_wrapper, "tags_object");
    register_object(&public_data_wrapper, "public_data_object");
    register_object(&private_data_wrapper, "private_data_object");

    for (int i = 0; i < 10; i++) {
        register_object(&peers_elements_wrapper[i], ("peer_object_" + std::to_string(i)).c_str());
        register_object(&peers_public_wrapper[i],
                        ("peer_public_data_" + std::to_string(i)).c_str());
        register_object(&peers_private_wrapper[i],
                        ("peer_private_data_" + std::to_string(i)).c_str());
        register_object(&peers_user_wrapper[i], ("peer_user_data_" + std::to_string(i)).c_str());
    }

    // lobby
    lua_pushlightuserdata(L, &lobby_wrapper);
    luaL_getmetatable(L, "SharedMetatable");
    lua_setmetatable(L, -2);
    lua_setfield(L, LUA_REGISTRYINDEX, "lobby_global");
    luaopen_lobby(L);
    luaopen_system(L);
}

void ScriptLua::open() {
    if (script_language != "lua") {
        enabled = false;
        return;
    }
    L = luaL_newstate();
    std::string base_path = scripts_folder + "/" + folder_name;
    lua_pushstring(L, base_path.c_str());
    lua_setfield(L, LUA_REGISTRYINDEX, "base_path");

    if (!L) {
        enabled = false;
        return;
    }
    INIReader config_reader(base_path + "/config.ini");
    autoreload = config_reader.GetBoolean("script", "autoreload", false);
    setupState(L);
    set_lua_metatables();
    bool success = runFile(L, (base_path + "/" + script_entrypoint).c_str());
    if (!success) {
        enabled = true;
        // enable with error
        return;
    }
    enabled = true;
}

AnyElement ScriptLua::func_call(std::string &func_name, boost::container::vector<AnyElement> &args,
                                std::string &peer_id, std::string &lobby_id, std::string &game_id,
                                bool &has_error) {
    if (!enabled) {
        return AnyElement{"Lua script is not enabled"};
    }
    if (autoreload) {
        close();
        open();
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
        return AnyElement{"Function not found. " + func_name};
    }

    // Push arguments onto the Lua stack
    for (int i = 0; i < args.size(); i++) {
        push_lua_value(L, args[i]);
    }
    int status = lua_pcall(L, args.size(), LUA_MULTRET, 0);
    if (status != 0) {
        has_error = true;
        const char *msg = lua_tostring(L, -1);
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
    enabled = false;
}
