#include "script_lua.h"

#include <filesystem>

#include "game_thread.h"

static AnyElement decode_luavalue(lua_State *L, int idx);

static AnyElement decode_luatable(lua_State *L, int idx) {
    std::unordered_map<std::string, AnyElement> result_dict;
    std::vector<AnyElement> result_array;
    if (!lua_istable(L, idx)) {
        return AnyElement{std::monostate{}};
    }
    lua_pushnil(L);
    bool is_dict = true;
    while (lua_next(L, idx) != 0) {
        if (lua_isnumber(L, -2)) {
            auto key = lua_tointeger(L, -2);
            result_array.push_back(decode_luavalue(L, -1));
        } else if (lua_isstring(L, -2)) {
            lua_pushvalue(L, -2);
            std::string key = lua_tostring(L, -1);
            if (key.size() == 0) {
                luaL_error(L, "empty key");
                lua_settop(L, 0);
                return AnyElement{std::monostate{}};
            }
            if (key[0] >= '0' && key[0] <= '9' && !is_dict) {
                is_dict = false;
                int num = std::stoi(key);
                if (num >= 1) {
                    result_array.push_back(decode_luavalue(L, -1));
                }
            } else {
                result_dict.emplace(key, decode_luavalue(L, -2));
            }
            lua_pop(L, 1);
        } else {
            luaL_error(L, "invalid key type");
            lua_settop(L, 0);
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
        case LUA_TNUMBER:
            return AnyElement{lua_tonumber(L, idx)};
        case LUA_TSTRING:
            return AnyElement{lua_tostring(L, idx)};
        case LUA_TTABLE:
            return decode_luatable(L, lua_gettop(L));
        default:
            return AnyElement{std::monostate{}};
    }
}

static void push_lua_value(lua_State *L, const AnyElement &value);

static void push_table(lua_State *L, const std::vector<AnyElement> &array) {
    lua_createtable(L, array.size(), 0);
    for (size_t i = 0; i < array.size(); ++i) {
        lua_pushinteger(L, i + 1);
        push_lua_value(L, array[i]);
        lua_settable(L, -3);
    }
}

static void push_table(lua_State *L, const std::unordered_map<std::string, AnyElement> &table) {
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
                // Use string view to avoid copying string contents.
                std::string_view str_view(v);
                lua_pushlstring(L, str_view.data(), str_view.size());
            } else if constexpr (std::is_same_v<T, std::vector<AnyElement>>) {
                push_table(L, v);
            } else if constexpr (std::is_same_v<T, std::unordered_map<std::string, AnyElement>>) {
                push_table(L, v);
            } else {
                lua_pushnil(L);
            }
        },
        value.value);
}

static int start_timer(lua_State *L) {
    const char *timer_id = luaL_checkstring(L, 1);
    int duration = luaL_checkinteger(L, 2);
    if (duration < 1 || duration > 300) {
        luaL_error(L, "Timer duration must be between 1 second and 5 minutes.");
        return 0;
    }
    std::vector<AnyElement> args;
    // Get the additional parameters sent with the timer
    for (int i = 3; i <= lua_gettop(L); ++i) {
        args.push_back(decode_luavalue(L, i));
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
    game.timer_data.emplace(timer_id, TimerData{
                                          .id = timer_id,
                                          .lobby_id = lobby_id,
                                          .game_id = game_id,
                                          .end_time = duration * 1000 + get_time_now(),
                                          .args = args,
                                      });

    return 0;
}

static int stop_timer(lua_State *L) {
    const char *timer_id = luaL_checkstring(L, 1);
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
    game.timer_data.erase(timer_id);
    return 0;
}

static int get_order_of_element(const std::set<std::string> &ordered_set, const std::string &key) {
    auto it = ordered_set.find(key);
    if (it != ordered_set.end()) {
        return std::distance(ordered_set.begin(), it);
    }
    return -1;
}

static std::string get_element_at_index(const std::set<std::string> &ordered_set, int index) {
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
            lobby.sealed = lua_toboolean(L, 3);
            lobby.sealed_dirty = true;
        }
    } else if (info->name == "tags") {
        lobby.tags[key] = decode_luavalue(L, 3);
        lobby.tags_dirty = true;
    } else if (info->name == "public_data") {
        lobby.public_data[key] = decode_luavalue(L, 3);
        lobby.public_data_dirty = true;
    } else if (info->name == "private_data") {
        lobby.private_data[key] = decode_luavalue(L, 3);
        lobby.private_data_dirty = true;
    } else if (info->name == "peer_public_data") {
        auto peer_id = get_element_at_index(lobby.peer_ids, info->idx);
        auto &peer = game.peers[peer_id];
        peer.public_data[key] = decode_luavalue(L, 3);
        peer.public_data_dirty = true;
    } else if (info->name == "peer_private_data") {
        auto peer_id = get_element_at_index(lobby.peer_ids, info->idx);
        auto &peer = game.peers[peer_id];
        peer.private_data[key] = decode_luavalue(L, 3);
        peer.private_data_dirty = true;
    }

    return 0;
}

static int lobby_index(lua_State *L) {
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

    const char *key = luaL_checkstring(L, 2);

    if (info->name == "lobby") {
        if (strcmp(key, "id") == 0) {
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

void ScriptLua::set_lua_metatables() {
    // game thread
    lua_pushlightuserdata(L, game_thread);
    lua_setfield(L, LUA_REGISTRYINDEX, "game_thread");

    // shared metatable
    luaL_newmetatable(L, "SharedMetatable");

    lua_pushstring(L, "__index");
    lua_pushcfunction(L, lobby_index);
    lua_settable(L, -3);
    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, lobby_newindex);
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
    lua_setglobal(L, "lobby");
}

int lua_print_to_file(lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "logs_path");
    std::string logs_path = lua_tostring(L, -1);
    lua_pop(L, 1);
    std::ofstream outfile(logs_path, std::ios_base::app);
    if (!outfile.is_open()) {
        luaL_error(L, "Failed to open output file");
        return 0;
    }

    int n = lua_gettop(L);  // Number of arguments
    for (int i = 1; i <= n; ++i) {
        if (i > 1) {
            outfile << "\t";  // Add tab between arguments
        }

        // Check argument type and convert it to string
        if (lua_isstring(L, i)) {
            outfile << lua_tostring(L, i);
        } else if (lua_isnumber(L, i)) {
            outfile << lua_tonumber(L, i);
        } else {
            outfile << lua_tostring(L, i);
        }
    }

    outfile << "\n";  // New line after printing all arguments

    // Close the file
    outfile.close();

    return 0;  // Return nothing to Lua
}

static void setup_print_redirect(lua_State *L) {
    lua_pushcfunction(L, lua_print_to_file);
    lua_setglobal(L, "print");
}

void ScriptLua::open() {
    if (script_language != "lua") {
        enabled = false;
        return;
    }
    L = luaL_newstate();
    if (!L) {
        enabled = false;
        return;
    }
    INIReader config_reader(scripts_folder + "/" + folder_name + "/config.ini");
    autoreload = config_reader.GetBoolean("script", "autoreload", false);
    luaL_openlibs(L);

    lua_pushcfunction(L, &start_timer);
    lua_setglobal(L, "start_timer");

    lua_pushcfunction(L, &stop_timer);
    lua_setglobal(L, "stop_timer");

    set_lua_metatables();

    // setup_print_redirect(L);
    luaL_dostring(
        L, ("package.path = \"" + scripts_folder + "/" + folder_name + "/?.lua;\" .. package.path")
               .c_str());
    luaL_dofile(L, (scripts_folder + "/" + folder_name + "/" + script_entrypoint).c_str());
    enabled = true;
}

AnyElement ScriptLua::func_call(std::string &func_name, std::vector<AnyElement> &args,
                                std::string &lobby_id, std::string &game_id, bool &has_error) {
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

    lua_pushstring(L, lobby_id.c_str());
    lua_setfield(L, LUA_REGISTRYINDEX, "lobby_id");

    lua_pushstring(L, game_id.c_str());
    lua_setfield(L, LUA_REGISTRYINDEX, "game_id");

    lua_getglobal(L, func_name.c_str());

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
