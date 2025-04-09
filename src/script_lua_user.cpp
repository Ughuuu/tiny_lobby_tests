#include "script_lua_user.h"
#include "script_lua.h"
#include "game_thread.h"
#include "luacode.h"
#include "lualib.h"
// Add these new structures at the top of the file
struct DataUserdata {
    enum DataType {
        LOBBY_PUBLIC,
        LOBBY_PRIVATE,
        PEER_PUBLIC,
        PEER_PRIVATE,
        PEER_USER,
        NESTED_MAP
    } type;
    
    GameThread* game_thread;
    std::string game_id;
    std::string lobby_id;
    std::string peer_id;
    std::string current_key;
};

static std::unordered_map<void*, DataUserdata> active_userdata;

// Helper functions
static auto& get_data_container(DataUserdata* ud) {
    auto& game = ud->game_thread->games[ud->game_id];
    
    if (ud->type == DataUserdata::NESTED_MAP) {
        auto& parent = [&]() -> auto& {
            switch (active_userdata.at(ud).type) {
                case DataUserdata::LOBBY_PUBLIC: return game.lobbies[ud->lobby_id].public_data;
                case DataUserdata::LOBBY_PRIVATE: return game.lobbies[ud->lobby_id].private_data;
                case DataUserdata::PEER_PUBLIC: return game.peers[ud->peer_id].public_data;
                case DataUserdata::PEER_PRIVATE: return game.peers[ud->peer_id].private_data;
                case DataUserdata::PEER_USER: return game.peers[ud->peer_id].user_data;
                default: throw std::runtime_error("Invalid data type");
            }
        }();
        
        return std::get<boost::container::flat_map<std::string, AnyElement>>(
            parent[ud->current_key].value
        );
    }
    
    switch (ud->type) {
        case DataUserdata::LOBBY_PUBLIC: return game.lobbies[ud->lobby_id].public_data;
        case DataUserdata::LOBBY_PRIVATE: return game.lobbies[ud->lobby_id].private_data;
        case DataUserdata::PEER_PUBLIC: return game.peers[ud->peer_id].public_data;
        case DataUserdata::PEER_PRIVATE: return game.peers[ud->peer_id].private_data;
        case DataUserdata::PEER_USER: return game.peers[ud->peer_id].user_data;
        default: throw std::runtime_error("Invalid data type");
    }
}

static void mark_data_dirty(DataUserdata* ud) {
    auto& game = ud->game_thread->games[ud->game_id];
    
    switch (ud->type) {
        case DataUserdata::LOBBY_PUBLIC:
            game.lobbies[ud->lobby_id].public_data_dirty = true;
            break;
        case DataUserdata::LOBBY_PRIVATE:
            game.lobbies[ud->lobby_id].private_data_dirty = true;
            break;
        case DataUserdata::PEER_PUBLIC:
        case DataUserdata::PEER_PRIVATE:
        case DataUserdata::PEER_USER:
            game.peers[ud->peer_id].public_data_dirty = true;
            break;
        case DataUserdata::NESTED_MAP:
            mark_data_dirty(&active_userdata.at(ud));
            break;
    }
}

// New userdata methods
static int data_userdata_index(lua_State* L) {
    auto* ud = static_cast<DataUserdata*>(luaL_checkudata(L, 1, "DataMetatable"));
    const char* key = luaL_checkstring(L, 2);
    
    auto& container = get_data_container(ud);
    
    if (container.find(key) != container.end()) {
        const auto& value = container[key];
        
        if (std::holds_alternative<boost::container::flat_map<std::string, AnyElement>>(value.value)) {
            auto* new_ud = static_cast<DataUserdata*>(
                lua_newuserdata(L, sizeof(DataUserdata))
            );
            *new_ud = *ud;
            new_ud->current_key = key;
            new_ud->type = DataUserdata::NESTED_MAP;
            
            active_userdata[new_ud] = *new_ud;
            
            luaL_getmetatable(L, "DataMetatable");
            lua_setmetatable(L, -2);
            return 1;
        }
        
        push_lua_value(L, value);
        return 1;
    }
    
    return 0;
}

static int data_userdata_newindex(lua_State* L) {
    auto* ud = static_cast<DataUserdata*>(luaL_checkudata(L, 1, "DataMetatable"));
    const char* key = luaL_checkstring(L, 2);
    AnyElement value = decode_luavalue(L, 3);
    
    auto& container = get_data_container(ud);
    container[key] = value;
    mark_data_dirty(ud);
    
    return 0;
}

static int data_userdata_gc(lua_State* L) {
    auto* ud = static_cast<DataUserdata*>(luaL_checkudata(L, 1, "DataMetatable"));
    active_userdata.erase(ud);
    return 0;
}

void create_data_userdata(lua_State* L, GameThread* game_thread,
                         const std::string& game_id,
                         const std::string& lobby_id,
                         const std::string& peer_id,
                         DataUserdata::DataType type) {
    auto* ud = static_cast<DataUserdata*>(
        lua_newuserdata(L, sizeof(DataUserdata))
    );
    *ud = DataUserdata{
        type,
        game_thread,
        game_id,
        lobby_id,
        peer_id,
        ""
    };
    
    active_userdata[ud] = *ud;
    luaL_getmetatable(L, "DataMetatable");
    lua_setmetatable(L, -2);
}

void register_data_metatable(lua_State* L) {
    luaL_newmetatable(L, "DataMetatable");
    
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, data_userdata_index);
    lua_settable(L, -3);
    
    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, data_userdata_newindex);
    lua_settable(L, -3);
    
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, data_userdata_gc);
    lua_settable(L, -3);
    
    lua_pop(L, 1);
}

// Modified lobby_index
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
        if (strcmp(key, "public_data") == 0) {
            create_data_userdata(L, game_thread, game_id, lobby_id, peer_id, 
                               DataUserdata::LOBBY_PUBLIC);
            return 1;
        } else if (strcmp(key, "private_data") == 0) {
            create_data_userdata(L, game_thread, game_id, lobby_id, peer_id,
                               DataUserdata::LOBBY_PRIVATE);
            return 1;
        }
        // ... rest of lobby fields ...
    } else if (info->name == "peer_object") {
        auto peer_id = get_element_at_index(lobby.peer_ids, info->idx);
        if (strcmp(key, "public_data") == 0) {
            create_data_userdata(L, game_thread, game_id, lobby_id, peer_id,
                               DataUserdata::PEER_PUBLIC);
            return 1;
        } else if (strcmp(key, "private_data") == 0) {
            create_data_userdata(L, game_thread, game_id, lobby_id, peer_id,
                               DataUserdata::PEER_PRIVATE);
            return 1;
        } else if (strcmp(key, "user_data") == 0) {
            create_data_userdata(L, game_thread, game_id, lobby_id, peer_id,
                               DataUserdata::PEER_USER);
            return 1;
        }
        // ... rest of peer fields ...
    }
    
    // ... rest of existing index logic ...
}

// Remove the static wrapper arrays and modify set_lua_metatables
void ScriptLua::set_lua_metatables() {
    lua_pushlightuserdata(L, game_thread);
    lua_setfield(L, LUA_REGISTRYINDEX, "game_thread");

    // Register the new data metatable
    register_data_metatable(L);

    // Shared metatable for other objects
    luaL_newmetatable(L, "SharedMetatable");
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, lobby_index);
    lua_settable(L, -3);
    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, lobby_newindex);
    lua_settable(L, -3);
    lua_pop(L, 1);

    // Register basic objects
    auto register_object = [&](void *userdata, const char *name) {
        lua_pushlightuserdata(L, userdata);
        luaL_getmetatable(L, "SharedMetatable");
        lua_setmetatable(L, -2);
        lua_setfield(L, LUA_REGISTRYINDEX, name);
    };

    register_object(&tags_wrapper, "tags_object");
    register_object(&public_data_wrapper, "public_data_object");
    register_object(&private_data_wrapper, "private_data_object");
    register_object(&lobby_wrapper, "lobby_global");

    luaopen_lobby(L);
    luaopen_system(L);
}
