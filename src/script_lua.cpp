#include "script_lua.h"
#include "game_thread.h"
#include <sol/sol.hpp>

static AnyElement decode_luavalue(lua_State *L,int idx);

static AnyElement decode_luatable(lua_State *L, int idx) {
    std::unordered_map<std::string, AnyElement> result_dict;
    std::vector<AnyElement> result_array;
    if (!lua_istable(L, idx)) {
        return AnyElement{std::monostate{}};
    }
    lua_pushnil(L);
    bool is_dict = false;
    while (lua_next(L, idx) != 0) {
        if (lua_isstring(L, -2)) {
            lua_pushvalue(L, -2);
            std::string key = lua_tostring(L, -1);
            lua_pop(L, 1);
            result_dict.emplace(key, decode_luavalue(L, -1));
            is_dict = true;
        } else if (lua_isnumber(L, -2)) {
            auto key = lua_tointeger(L, -2);
            result_array.push_back(decode_luavalue(L, -1));
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

static AnyElement decode_luavalue(lua_State *L,int idx) {
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

static void push_lua_value(lua_State *L, const AnyElement &value) {
    if (std::holds_alternative<std::monostate>(value.value)) {
        lua_pushnil(L);
    } else if (std::holds_alternative<bool>(value.value)) {
        lua_pushboolean(L, std::get<bool>(value.value));
    } else if (std::holds_alternative<int64_t>(value.value)) {
        lua_pushinteger(L, std::get<int64_t>(value.value));
    } else if (std::holds_alternative<double>(value.value)) {
        lua_pushnumber(L, std::get<double>(value.value));
    } else if (std::holds_alternative<std::string>(value.value)) {
        std::string str = std::get<std::string>(value.value);
        lua_pushlstring(L, str.c_str(), str.size());
    } else if (std::holds_alternative<std::vector<AnyElement>>(value.value)) {
        auto array = std::get<std::vector<AnyElement>>(value.value);
        lua_createtable(L, array.size(), 0);

        for (size_t i = 0; i < array.size(); ++i) {
            lua_pushinteger(L, i + 1);
            push_lua_value(L, array[i]);
            lua_settable(L, -3);
        }
    } else if (std::holds_alternative<std::unordered_map<std::string, AnyElement>>(value.value)) {
        auto table = std::get<std::unordered_map<std::string, AnyElement>>(value.value);
        lua_createtable(L, table.size(), 0);

        for (const auto &pair : table) {
            lua_pushstring(L, pair.first.c_str());
            push_lua_value(L, pair.second);
            lua_settable(L, -3);
        }
    } else {
        lua_pushnil(L);
    }
}

static int start_timer(lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "lobby_id");
    std::string lobby_id = lua_tostring(L, -1);
    lua_pop(L, 1);

    const char *timer_id = luaL_checkstring(L, 1);
    int duration = luaL_checkinteger(L, 2);
    if (duration < 1 || duration > 300) {
        luaL_error(L, "Timer duration must be between 1 second and 5 minutes.");
        return 0;
    }
    // Get the additional parameters sent with the timer
    //for (int i=0;i<lua_gettop(L))

    // Here, you would create a native timer, for example:
    printf("Starting timer '%s' for %d seconds.\n", timer_id, duration);

    return 0;
}

static int stop_timer(lua_State *L) {
    const char *timer_id = luaL_checkstring(L, 1);
    // Code to stop the timer (from a timer map or data structure)
    printf("Stopping timer '%s'.\n", timer_id);
    return 0;
}

static int lobby_get(lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "game_id");
    std::string game_id = lua_tostring(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, "lobby_id");
    std::string lobby_id = lua_tostring(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, "game_thread");
    GameThread* game_thread = static_cast<GameThread*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    auto &game = game_thread->games[game_id];
    auto &lobby = game.lobbies[lobby_id];
    std::unordered_map<std::string, AnyElement> peers_dict{};
    for (auto& peer_id : lobby.peer_ids) {
        auto &peer = game.peers[peer_id];
        std::unordered_map<std::string, AnyElement> peer_data;
        peer_data.emplace("id", peer.id);
        peer_data.emplace("order_id", peer.order_id);
        peer_data.emplace("ready", peer.ready);
        peer_data.emplace("public_data", peer.public_data);
        peer_data.emplace("private_data", peer.private_data);
        peer_data.emplace("user_data", peer.user_data);
        peer_data.emplace("disconnected", peer.disconnected);
        peers_dict.emplace(peer_id, peer_data);
    }
    std::unordered_map<std::string, AnyElement> lobby_dict;
    lobby_dict.emplace("id", lobby_id);
    lobby_dict.emplace("name", lobby.name);
    lobby_dict.emplace("host", lobby.host);
    lobby_dict.emplace("max_players", lobby.max_players);
    lobby_dict.emplace("create_time", int64_t(lobby.create_time));
    lobby_dict.emplace("game_id", game_id);
    lobby_dict.emplace("sealed", lobby.sealed);
    lobby_dict.emplace("tags", AnyElement{lobby.tags});
    lobby_dict.emplace("public_data", AnyElement{lobby.public_data});
    lobby_dict.emplace("private_data", AnyElement{lobby.private_data});
    lobby_dict.emplace("peers", AnyElement{peers_dict});
    push_lua_value(L, AnyElement{lobby_dict});
    return 1;
}

static int lobby_save(lua_State *L) {
    AnyElement args{};
    if (lua_gettop(L) != 0) {
        args = decode_luavalue(L, 1);
        lua_settop(L, 0);
    }
    lua_getfield(L, LUA_REGISTRYINDEX, "game_id");
    std::string game_id = lua_tostring(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, "lobby_id");
    std::string lobby_id = lua_tostring(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, "game_thread");
    GameThread* game_thread = static_cast<GameThread*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    auto &game = game_thread->games[game_id];
    auto &lobby = game.lobbies[lobby_id];
    // update lobby and peers with data
    auto lobby_map = std::get_if<std::unordered_map<std::string, AnyElement>>(&args.value);
    if (lobby_map) {
        if (auto sealed = std::get_if<bool>(&(*lobby_map)["sealed"].value)) {
            lobby.sealed = *sealed;
        }
        if (auto tags = std::get_if<std::unordered_map<std::string, AnyElement>>(&(*lobby_map)["tags"].value)) {
            lobby.tags = *tags;
        }
        if (auto public_data = std::get_if<std::unordered_map<std::string, AnyElement>>(&(*lobby_map)["public_data"].value)) {
            lobby.public_data = *public_data;
        }
        if (auto private_data = std::get_if<std::unordered_map<std::string, AnyElement>>(&(*lobby_map)["private_data"].value)) {
            lobby.private_data = *private_data;
        }
        auto peers_data = std::get_if<std::unordered_map<std::string, AnyElement>>(&(*lobby_map)["peers"].value);
        if (peers_data) {
            for (auto &peer_pair : *peers_data) {
                auto peer_id = peer_pair.first;
                auto peer_data = std::get_if<std::unordered_map<std::string, AnyElement>>(&peer_pair.second.value);
                if (peer_data) {
                    auto &peer = game.peers[peer_id];
                    if (auto ready = std::get_if<bool>(&(*peer_data)["ready"].value)) {
                        peer.ready = *ready;
                    }
                    if (auto public_data = std::get_if<std::unordered_map<std::string, AnyElement>>(&(*peer_data)["public_data"].value)) {
                        peer.public_data = *public_data;
                    }
                    if (auto private_data = std::get_if<std::unordered_map<std::string, AnyElement>>(&(*peer_data)["private_data"].value)) {
                        peer.private_data = *private_data;
                    }
                    if (auto user_data = std::get_if<std::unordered_map<std::string, AnyElement>>(&(*peer_data)["user_data"].value)) {
                        peer.user_data = *user_data;
                    }
                }
            }
        }
    }
    // notify peers about the changes
    game_thread->notify_lobby_changes(game_id, lobby_id);
    return 0;
}

static int luaopen_lobby(lua_State* L)
{
    //create table with 4 entries
    lua_createtable(L, 0, 4);

    //set key value pairs of the table
    lua_pushstring(L, "start_timer");
    lua_pushcfunction(L, &start_timer);
    lua_settable(L, -3);

    lua_pushstring(L, "stop_timer");
    lua_pushcfunction(L, &stop_timer);
    lua_settable(L, -3);

    lua_pushstring(L, "get");
    lua_pushcfunction(L, &lobby_get);
    lua_settable(L, -3);

    lua_pushstring(L, "save");
    lua_pushcfunction(L, &lobby_save);
    lua_settable(L, -3);

    return 1;
}

int lua_print_to_file(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "logs_path");
    std::string logs_path = lua_tostring(L, -1);
    lua_pop(L, 1);
    std::ofstream outfile(logs_path, std::ios_base::app);
    if (!outfile.is_open()) {
        luaL_error(L, "Failed to open output file");
        return 0;
    }

    int n = lua_gettop(L); // Number of arguments
    for (int i = 1; i <= n; ++i) {
        if (i > 1) {
            outfile << "\t"; // Add tab between arguments
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

    outfile << "\n"; // New line after printing all arguments

    // Close the file
    outfile.close();

    return 0; // Return nothing to Lua
}

static void setup_print_redirect(lua_State* L) {
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
    INIReader config_reader(scripts_folder +"/" + folder_name + "/config.ini");
    autoreload = config_reader.GetBoolean("script", "autoreload", false);
    luaL_openlibs(L);

    // register lobby module
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    lua_pushstring(L, "lobby");
    lua_pushcfunction(L, luaopen_lobby);
    lua_settable(L, -3);
    lua_pop(L, 2);

    // register game_thread
    lua_pushlightuserdata(L, game_thread);
    lua_setfield(L, LUA_REGISTRYINDEX, "game_thread");

    // setup_print_redirect(L);
    luaL_dostring(L, ("package.path = \"" + scripts_folder + "/" + folder_name + "/?.lua;\" .. package.path").c_str());
    luaL_dofile(L, (scripts_folder + "/" + folder_name + "/" + script_entrypoint).c_str());
    enabled = true;
}

AnyElement ScriptLua::func_call(std::string &func_name, std::vector<AnyElement> &args, std::string &lobby_id, std::string &game_id, bool &has_error) {
    if (!enabled) {
        return AnyElement{"Lua script is not enabled"};
    }
    if (autoreload) {
        //close();
        //open();
    }
    if (!enabled) {
        return AnyElement{"Lua script is not enabled"};
    }
    // Push error handler (debug.traceback)
    int error_func_index = 0;
    if (false) {
        lua_getglobal(L, "debug");
        lua_getfield(L, -1, "traceback");
        lua_remove(L, -2); // remove 'debug', leave traceback on top
        error_func_index = lua_gettop(L);
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
    int status = lua_pcall(L, args.size(), LUA_MULTRET, error_func_index);
    if (status != 0) {
        has_error = true;
        const char *msg = lua_tostring(L, -1);
        std::string error_msg = msg ? msg : "Unknown Lua error";
        lua_settop(L, 0);
        return AnyElement{error_msg};
    }
    if (error_func_index != 0) {
        lua_remove(L, error_func_index);
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
