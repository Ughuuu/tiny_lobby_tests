#include "script_lua_functions.h"

#include <filesystem>
#include <variant>

#include "game_thread.h"
#include "luacode.h"
#include "lualib.h"
#include "script_as_http.h"
#include "script_lua.h"

AnyElement decode_luavalue(lua_State *L, int idx) {
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
            return decode_luatable(L, idx);
        default:
            return AnyElement{std::monostate{}};
    }
}
AnyElement decode_luatable(lua_State *L, int idx) {
    boost::container::flat_map<std::string, AnyElement> result_dict;
    boost::container::vector<AnyElement> result_array;
    if (!lua_istable(L, idx)) {
        return AnyElement{std::monostate{}};
    }
    int abs_idx = lua_absindex(L, idx);
    lua_pushnil(L);
    bool is_dict = true;
    while (lua_next(L, abs_idx) != 0) {
        if (lua_isnumber(L, -2)) {
            auto key = lua_tointeger(L, -2);
            result_array.push_back(decode_luavalue(L, -1));
            is_dict = false;
        } else if (lua_isstring(L, -2)) {
            std::string key = lua_tostring(L, -2);
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
                auto value_decoded = decode_luavalue(L, -1);
                // do not put nil values in dictionary
                if (!std::holds_alternative<std::monostate>(value_decoded.value)) {
                    result_dict.emplace(key, value_decoded);
                }
            }
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

void push_table(lua_State *L, const boost::container::vector<AnyElement> &array) {
    lua_createtable(L, array.size(), 0);
    for (size_t i = 0; i < array.size(); ++i) {
        lua_pushinteger(L, i + 1);
        push_lua_value(L, array[i]);
        lua_settable(L, -3);
    }
}

void push_table(lua_State *L, const boost::container::flat_map<std::string, AnyElement> &table) {
    lua_createtable(L, table.size(), 0);
    for (const auto &pair : table) {
        lua_pushstring(L, pair.first.c_str());
        push_lua_value(L, pair.second);
        lua_settable(L, -3);
    }
}

void push_lua_value(lua_State *L, const AnyElement &value) {
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

int start_timer(lua_State *L) {
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

int stop_timer(lua_State *L) {
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

int notify(lua_State *L) {
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
    game_thread->notify_peer(game, lobby_id, peer_id, notification_object);
    return 0;
}

int broadcast_chat(lua_State *L) {
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

int get_time(lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "game_thread");
    GameThread *game_thread = static_cast<GameThread *>(lua_touserdata(L, -1));
    lua_pushstring(L, std::to_string(game_thread->get_time()).c_str());
    return 1;
}

int http_request(lua_State *L) {
    if (lua_gettop(L) < 2 || lua_gettop(L) > 5) {
        luaL_error(L, "Expected 2 argument min. Max 5.");
        return 0;
    }
    std::string method = luaL_checkstring(L, 1);
    std::string url = luaL_checkstring(L, 2);
    AnyElement query_params{boost::container::vector<AnyElement>{}};
    if (lua_gettop(L) >= 3) {
        query_params = decode_luavalue(L, 3);
    }
    AnyElement headers{boost::container::vector<AnyElement>{}};
    if (lua_gettop(L) >= 4) {
        headers = decode_luavalue(L, 4);
    }
    std::string body;
    if (lua_gettop(L) >= 5) {
        body = luaL_checkstring(L, 5);
    }
    boost::container::vector<AnyElement> headers_vec;
    boost::container::vector<AnyElement> query_params_vec;
    if (auto *headers_arr = std::get_if<boost::container::vector<AnyElement>>(&headers.value)) {
        headers_vec = *headers_arr;
    }
    if (auto *query_params_arr =
            std::get_if<boost::container::vector<AnyElement>>(&query_params.value)) {
        query_params_vec = *query_params_arr;
    }

    std::vector<std::pair<std::string, std::string>> query_params_input;
    std::vector<std::pair<std::string, std::string>> headers_input;
    for (size_t i = 0; i + 1 < query_params_vec.size(); i += 2) {
        if (auto key = std::get_if<std::string>(&query_params_vec[i].value)) {
            if (auto val = std::get_if<std::string>(&query_params_vec[i + 1].value)) {
                query_params_input.emplace_back(*key, *val);
            }
        }
    }
    for (size_t i = 0; i + 1 < headers_vec.size(); i += 2) {
        if (auto key = std::get_if<std::string>(&headers_vec[i].value)) {
            if (auto val = std::get_if<std::string>(&headers_vec[i + 1].value)) {
                headers_input.emplace_back(*key, *val);
            }
        }
    }
    try {
        auto res = request(method, url, query_params_input, headers_input, body);
        lua_pushinteger(L, res.result_int());
        lua_pushstring(L, res.body().c_str());
    } catch (const std::exception &e) {
        lua_pushinteger(L, 500);
        lua_pushstring(L, (std::string("Error: ") + e.what()).c_str());
    }
    return 2;
}

int DecodeJSON_from_string(lua_State *L) {
    if (lua_gettop(L) < 1) {
        luaL_error(L, "Expected 1 argument.");
        return 0;
    }
    std::string json = luaL_checkstring(L, 1);
    yyjson_doc *doc = yyjson_read(json.c_str(), json.length(), 0);
    if (!doc) return 0;

    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!root) {
        yyjson_doc_free(doc);
        return 0;
    }

    AnyElement root_value;
    std::string error = decode_value(root, root_value);
    if (!error.empty()) {
        yyjson_doc_free(doc);
        asIScriptContext *ctx = asGetActiveContext();
        if (ctx) ctx->SetException(("JSON parse error: " + error).c_str());
        return 0;
    }
    push_lua_value(L, root_value);
    yyjson_doc_free(doc);
    return 1;
}

// Encode to string
int EncodeJSON_to_string(lua_State *L) {
    if (lua_gettop(L) < 1) {
        luaL_error(L, "Expected 1 argument.");
        return 0;
    }

    AnyElement val = decode_luavalue(L, 1);
    push_lua_value(L, AnyElement{val.to_string().c_str()});
    return 1;
}

int get_lobby(lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "peer_id");
    std::string caling_peer_id = lua_tostring(L, -1);
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
    std::string empty_string;

    create_lobby_userdata(L, game_thread, game_id, lobby_id, caling_peer_id, empty_string,
                          LobbyUserdata::LobbyType::LOBBY_ROOT);
    return 1;
}

int lua_require(lua_State *L) {
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

void setup_lua_state(lua_State *L) {
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

boost::container::flat_set<std::string> run_lua_file(lua_State *L, std::string name) {
    std::ifstream file(name);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    std::string chunkname = "@" + std::string(name);
    size_t bytecodeSize = 0;
    char *bytecode = luau_compile(source.c_str(), source.size(), NULL, &bytecodeSize);
    int status = luau_load(L, chunkname.c_str(), bytecode, bytecodeSize, 0);
    free(bytecode);
    boost::container::flat_set<std::string> enabled_functions;

    if (status != LUA_OK) {
        std::cerr << "Load error: " << lua_tostring(L, -1) << "\n";
        return enabled_functions;
    } else {
        // Run the loaded chunk
        if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
            std::cerr << "Runtime error: " << lua_tostring(L, -1) << "\n";
            return enabled_functions;
        }
        if (!lua_istable(L, -1)) {
            std::cerr << "Script must return a table " << name << "\n";
            return enabled_functions;
        }

        std::vector<std::string> expected_functions = {"_on_create", "_on_join", "_on_chat",
                                                       "_on_tags",   "_on_kick", "_on_ready",
                                                       "_on_seal",   "_on_left"};

        for (const auto &func_name : expected_functions) {
            lua_getfield(L, -1, func_name.c_str());
            if (lua_isfunction(L, -1)) {
                enabled_functions.insert(func_name);
            }
            lua_pop(L, 1);
        }
        lua_setfield(L, LUA_REGISTRYINDEX, "main");
    }
    return enabled_functions;
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

    lua_pushcfunction(L, get_time, "get_time_since_epoch");
    lua_setfield(L, -2, "get_time_since_epoch");

    lua_pushcfunction(L, http_request, "http_request");
    lua_setfield(L, -2, "http_request");

    lua_pushcfunction(L, DecodeJSON_from_string, "decode_json");
    lua_setfield(L, -2, "decode_json");
    lua_pushcfunction(L, EncodeJSON_to_string, "encode_json");
    lua_setfield(L, -2, "encode_json");

    luaL_findtable(L, LUA_REGISTRYINDEX, "_MODULES", 1);
    lua_pushstring(L, "system");
    lua_pushvalue(L, -3);
    lua_settable(L, -3);
    lua_pop(L, 2);
}
