#include "script_lua.h"

#include <filesystem>
#include <variant>

#include "game_thread.h"
#include "luacode.h"
#include "lualib.h"
#include "script_lua_functions.h"

void create_lobby_userdata(lua_State* L, GameThread* game_thread, const std::string& game_id,
                           const std::string& lobby_id, const std::string& calling_peer_id,
                           const std::string& peer_id,
                           const boost::container::vector<LobbyUserDataKey>& keys,
                           LobbyUserdata::LobbyType type) {
    auto* ud = static_cast<LobbyUserdata*>(lua_newuserdata(L, sizeof(LobbyUserdata)));
    *ud = LobbyUserdata{type, game_thread, game_id, lobby_id, calling_peer_id, peer_id, keys};

    luaL_getmetatable(L, "LobbyMetatable");
    lua_setmetatable(L, -2);
}
AnyElement* find_up_to_path(boost::container::flat_map<std::string, AnyElement>& dict,
                            const boost::container::vector<LobbyUserDataKey>& keys, int idx = 0);

AnyElement* find_up_to_idx(boost::container::vector<AnyElement>& vec,
                           const boost::container::vector<LobbyUserDataKey>& keys, int idx = 0) {
    if (idx >= keys.size()) {
        return nullptr;
    }

    const auto& key = keys[idx];

    if (key.idx >= vec.size()) {
        return nullptr;  // avoid out-of-bounds
    }

    AnyElement& value = vec[key.idx];

    if (idx == keys.size() - 1) {
        return &value;
    }
    if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(value.value)) {
        auto& nested_dict =
            std::get<boost::container::flat_map<std::string, AnyElement>>(value.value);
        return find_up_to_path(nested_dict, keys, idx + 1);
    }

    if (std::holds_alternative<boost::container::vector<AnyElement>>(value.value)) {
        auto& nested_vec = std::get<boost::container::vector<AnyElement>>(value.value);
        return find_up_to_idx(nested_vec, keys, idx + 1);
    }

    return &value;
}

AnyElement* find_up_to_path(boost::container::flat_map<std::string, AnyElement>& dict,
                            const boost::container::vector<LobbyUserDataKey>& keys, int idx) {
    if (idx >= keys.size()) {
        return nullptr;
    }

    const auto& key = keys[idx];

    auto it = dict.find(key.key);
    if (it == dict.end()) {
        return nullptr;
    }

    AnyElement& value = it->second;

    if (idx == keys.size() - 1) {
        return &value;
    }
    if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(value.value)) {
        auto& nested_dict =
            std::get<boost::container::flat_map<std::string, AnyElement>>(value.value);
        return find_up_to_path(nested_dict, keys, idx + 1);
    }

    if (std::holds_alternative<boost::container::vector<AnyElement>>(value.value)) {
        auto& nested_vec = std::get<boost::container::vector<AnyElement>>(value.value);
        return find_up_to_idx(nested_vec, keys, idx + 1);
    }

    return &value;
}

int string_to_int_or_minus1(const std::string& str) {
    if (str.size() == 0 || str[0] < '0' || str[0] > '9') {
        return -1;
    }
    try {
        return std::stoi(str);
    } catch (const std::invalid_argument&) {
        return -1;
    } catch (const std::out_of_range&) {
        return -1;
    }
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
    std::string empty_string;
    boost::container::vector<LobbyUserDataKey> keys = info->keys;

    switch (info->type) {
        case LobbyUserdata::LobbyType::LOBBY_ROOT: {
            if (strcmp(key, "sealed") == 0) {
                bool new_sealed = lua_toboolean(L, 3);
                if (new_sealed != lobby.sealed) {
                    lobby.sealed = new_sealed;
                    lobby.sealed_dirty = true;
                }
            } else if (strcmp(key, "public_data") == 0) {
                AnyElement new_public_data = decode_luavalue(L, 3);
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        new_public_data.value)) {
                    auto& new_public_data_dict =
                        std::get<boost::container::flat_map<std::string, AnyElement>>(
                            new_public_data.value);
                    if (lobby.public_data != new_public_data_dict) {
                        lobby.public_data = new_public_data_dict;
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
                    if (lobby.private_data != new_private_data_dict) {
                        lobby.private_data = new_private_data_dict;
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
                    if (lobby.tags != new_tags_dict) {
                        lobby.tags = new_tags_dict;
                        lobby.tags_dirty = true;
                    }
                }
            }
        } break;
        case LobbyUserdata::LobbyType::LOBBY_TAGS: {
            auto new_tag = decode_luavalue(L, 3);
            if (AnyElement* result = find_up_to_path(lobby.tags, keys, 0)) {
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        result->value)) {
                    auto& nested_dict =
                        std::get<boost::container::flat_map<std::string, AnyElement>>(
                            result->value);
                    nested_dict[key] = new_tag;
                    lobby.tags_dirty = true;
                } else if (std::holds_alternative<boost::container::vector<AnyElement>>(
                               result->value)) {
                    auto& nested_array =
                        std::get<boost::container::vector<AnyElement>>(result->value);
                    int idx = string_to_int_or_minus1(key);
                    if (idx != -1) {
                        if (idx >= nested_array.size()) {
                            nested_array.resize(idx + 1);
                        }
                        nested_array[idx] = new_tag;
                        lobby.tags_dirty = true;
                    }
                }
            }
        } break;
        case LobbyUserdata::LobbyType::LOBBY_PUBLIC_DATA: {
            auto new_public_data = decode_luavalue(L, 3);
            if (AnyElement* result = find_up_to_path(lobby.public_data, keys, 0)) {
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        result->value)) {
                    auto& nested_dict =
                        std::get<boost::container::flat_map<std::string, AnyElement>>(
                            result->value);
                    nested_dict[key] = new_public_data;
                    lobby.public_data_dirty = true;
                } else if (std::holds_alternative<boost::container::vector<AnyElement>>(
                               result->value)) {
                    auto& nested_array =
                        std::get<boost::container::vector<AnyElement>>(result->value);
                    int idx = string_to_int_or_minus1(key);
                    if (idx != -1) {
                        if (idx >= nested_array.size()) {
                            nested_array.resize(idx + 1);
                        }
                        nested_array[idx] = new_public_data;
                        lobby.public_data_dirty = true;
                    }
                }
            }
        } break;
        case LobbyUserdata::LobbyType::LOBBY_PRIVATE_DATA: {
            auto new_private_data = decode_luavalue(L, 3);
            if (AnyElement* result = find_up_to_path(lobby.private_data, keys, 0)) {
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        result->value)) {
                    auto& nested_dict =
                        std::get<boost::container::flat_map<std::string, AnyElement>>(
                            result->value);
                    nested_dict[key] = new_private_data;
                    lobby.private_data_dirty = true;
                } else if (std::holds_alternative<boost::container::vector<AnyElement>>(
                               result->value)) {
                    auto& nested_array =
                        std::get<boost::container::vector<AnyElement>>(result->value);
                    int idx = string_to_int_or_minus1(key);
                    if (idx != -1) {
                        if (idx >= nested_array.size()) {
                            nested_array.resize(idx + 1);
                        }
                        nested_array[idx] = new_private_data;
                        lobby.private_data_dirty = true;
                    }
                }
            }
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
                    if (peer.public_data != new_public_data_dict) {
                        peer.public_data = new_public_data_dict;
                        peer.public_data_dirty = true;
                    }
                }
            } else if (strcmp(key, "private_data") == 0) {
                AnyElement new_private_data = decode_luavalue(L, 3);
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        new_private_data.value)) {
                    auto& new_private_data_dict =
                        std::get<boost::container::flat_map<std::string, AnyElement>>(
                            new_private_data.value);
                    if (peer.private_data != new_private_data_dict) {
                        peer.private_data = new_private_data_dict;
                        peer.private_data_dirty = true;
                    }
                }
            }
        } break;
        case LobbyUserdata::LobbyType::PEER_PUBLIC_DATA: {
            auto& peer = game.peers[info->peer_id];
            auto new_public_data = decode_luavalue(L, 3);
            if (AnyElement* result = find_up_to_path(peer.public_data, keys, 0)) {
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        result->value)) {
                    auto& nested_dict =
                        std::get<boost::container::flat_map<std::string, AnyElement>>(
                            result->value);
                    nested_dict[key] = new_public_data;
                    peer.public_data_dirty = true;
                } else if (std::holds_alternative<boost::container::vector<AnyElement>>(
                               result->value)) {
                    auto& nested_array =
                        std::get<boost::container::vector<AnyElement>>(result->value);
                    int idx = string_to_int_or_minus1(key);
                    if (idx != -1) {
                        if (idx >= nested_array.size()) {
                            nested_array.resize(idx + 1);
                        }
                        nested_array[idx] = new_public_data;
                        peer.public_data_dirty = true;
                    }
                }
            }
        } break;
        case LobbyUserdata::LobbyType::PEER_PRIVATE_DATA: {
            auto& peer = game.peers[info->peer_id];
            auto new_private_data = decode_luavalue(L, 3);
            if (AnyElement* result = find_up_to_path(peer.private_data, keys, 0)) {
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        result->value)) {
                    auto& nested_dict =
                        std::get<boost::container::flat_map<std::string, AnyElement>>(
                            result->value);
                    nested_dict[key] = new_private_data;
                    peer.private_data_dirty = true;
                } else if (std::holds_alternative<boost::container::vector<AnyElement>>(
                               result->value)) {
                    auto& nested_array =
                        std::get<boost::container::vector<AnyElement>>(result->value);
                    int idx = string_to_int_or_minus1(key);
                    if (idx != -1) {
                        if (idx >= nested_array.size()) {
                            nested_array.resize(idx + 1);
                        }
                        nested_array[idx] = new_private_data;
                        peer.private_data_dirty = true;
                    }
                }
            }
        } break;
        case LobbyUserdata::LobbyType::PEER_USER_DATA: {
            auto& peer = game.peers[info->peer_id];
            auto new_user_data = decode_luavalue(L, 3);
            if (AnyElement* result = find_up_to_path(peer.user_data, keys, 0)) {
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        result->value)) {
                    auto& nested_dict =
                        std::get<boost::container::flat_map<std::string, AnyElement>>(
                            result->value);
                    nested_dict[key] = new_user_data;
                } else if (std::holds_alternative<boost::container::vector<AnyElement>>(
                               result->value)) {
                    auto& nested_array =
                        std::get<boost::container::vector<AnyElement>>(result->value);
                    int idx = string_to_int_or_minus1(key);
                    if (idx != -1) {
                        if (idx >= nested_array.size()) {
                            nested_array.resize(idx + 1);
                        }
                        nested_array[idx] = new_user_data;
                        peer.user_data_dirty = true;
                    }
                }
            }
        }
    }

    return 0;
}

static int lobby_index(lua_State* L) {
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
    std::string empty_string;
    boost::container::vector<LobbyUserDataKey> keys = info->keys;
    keys.push_back(LobbyUserDataKey{key, string_to_int_or_minus1(key)});

    switch (info->type) {
        case LobbyUserdata::LobbyType::LOBBY_ROOT: {
            if (strcmp(key, "calling_peer_id") == 0) {
                lua_pushstring(L, calling_peer_id.c_str());
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
                create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id,
                                      empty_string, info->keys,
                                      LobbyUserdata::LobbyType::LOBBY_TAGS);
                return 1;
            } else if (strcmp(key, "public_data") == 0) {
                create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id,
                                      empty_string, info->keys,
                                      LobbyUserdata::LobbyType::LOBBY_PUBLIC_DATA);
                return 1;
            } else if (strcmp(key, "private_data") == 0) {
                create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id,
                                      empty_string, info->keys,
                                      LobbyUserdata::LobbyType::LOBBY_PRIVATE_DATA);
                return 1;
            } else if (strcmp(key, "peers") == 0) {
                lua_newtable(L);

                for (const auto& peer_id : lobby.peer_ids) {
                    auto& peer = game.peers[peer_id];
                    create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id,
                                          peer_id, info->keys, LobbyUserdata::LobbyType::PEER_ROOT);
                    lua_setfield(L, -2, peer_id.c_str());
                }
                return 1;
            } else if (strcmp(key, "peers_count") == 0) {
                lua_pushinteger(L, lobby.peer_ids.size());
                return 1;
            }
        } break;
        case LobbyUserdata::LobbyType::LOBBY_TAGS: {
            if (AnyElement* result = find_up_to_path(lobby.tags, keys, 0)) {
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        result->value) ||
                    std::holds_alternative<boost::container::vector<AnyElement>>(result->value)) {
                    create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id,
                                          empty_string, keys, LobbyUserdata::LobbyType::LOBBY_TAGS);
                    return 1;
                }
                push_lua_value(L, *result);
                return 1;
            }
        } break;
        case LobbyUserdata::LobbyType::LOBBY_PUBLIC_DATA: {
            if (AnyElement* result = find_up_to_path(lobby.public_data, keys, 0)) {
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        result->value) ||
                    std::holds_alternative<boost::container::vector<AnyElement>>(result->value)) {
                    create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id,
                                          empty_string, keys,
                                          LobbyUserdata::LobbyType::LOBBY_PUBLIC_DATA);
                    return 1;
                }
                push_lua_value(L, *result);
                return 1;
            }
        } break;
        case LobbyUserdata::LobbyType::LOBBY_PRIVATE_DATA: {
            if (AnyElement* result = find_up_to_path(lobby.private_data, keys, 0)) {
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        result->value) ||
                    std::holds_alternative<boost::container::vector<AnyElement>>(result->value)) {
                    create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id,
                                          empty_string, keys,
                                          LobbyUserdata::LobbyType::LOBBY_PRIVATE_DATA);
                    return 1;
                }
                push_lua_value(L, *result);
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
            } else if (strcmp(key, "ready") == 0) {
                lua_pushboolean(L, peer.ready);
                return 1;
            } else if (strcmp(key, "disconnected") == 0) {
                lua_pushboolean(L, peer.disconnected);
                return 1;
            } else if (strcmp(key, "public_data") == 0) {
                create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id, peer.id,
                                      info->keys, LobbyUserdata::LobbyType::PEER_PUBLIC_DATA);
                return 1;
            } else if (strcmp(key, "private_data") == 0) {
                create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id, peer.id,
                                      info->keys, LobbyUserdata::LobbyType::PEER_PRIVATE_DATA);
                return 1;
            } else if (strcmp(key, "user_data") == 0) {
                create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id, peer.id,
                                      info->keys, LobbyUserdata::LobbyType::PEER_USER_DATA);
                return 1;
            }
        } break;
        case LobbyUserdata::LobbyType::PEER_PUBLIC_DATA: {
            auto& peer = game.peers[info->peer_id];
            if (AnyElement* result = find_up_to_path(peer.public_data, keys, 0)) {
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        result->value) ||
                    std::holds_alternative<boost::container::vector<AnyElement>>(result->value)) {
                    create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id,
                                          empty_string, keys,
                                          LobbyUserdata::LobbyType::PEER_PUBLIC_DATA);
                    return 1;
                }
                push_lua_value(L, *result);
                return 1;
            }
        } break;
        case LobbyUserdata::LobbyType::PEER_PRIVATE_DATA: {
            auto& peer = game.peers[info->peer_id];
            if (AnyElement* result = find_up_to_path(peer.private_data, keys, 0)) {
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        result->value) ||
                    std::holds_alternative<boost::container::vector<AnyElement>>(result->value)) {
                    create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id,
                                          empty_string, keys,
                                          LobbyUserdata::LobbyType::PEER_PRIVATE_DATA);
                    return 1;
                }
                push_lua_value(L, *result);
                return 1;
            }
        } break;
        case LobbyUserdata::LobbyType::PEER_USER_DATA: {
            auto& peer = game.peers[info->peer_id];
            if (AnyElement* result = find_up_to_path(peer.user_data, keys, 0)) {
                if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(
                        result->value) ||
                    std::holds_alternative<boost::container::vector<AnyElement>>(result->value)) {
                    create_lobby_userdata(L, game_thread, game_id, lobby_id, calling_peer_id,
                                          empty_string, keys,
                                          LobbyUserdata::LobbyType::PEER_USER_DATA);
                    return 1;
                }
                push_lua_value(L, *result);
                return 1;
            }
        } break;
    }
    return 0;
}

void ScriptLua::set_lua_metatables() {
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
    INIReader config_reader(base_path + "/config.ini");
    autoreload = config_reader.GetBoolean("script", "autoreload", false);
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
    enabled = false;
}
