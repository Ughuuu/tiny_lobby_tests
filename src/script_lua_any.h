#pragma once
#include "luacode.h"
#include "lualib.h"
#include "any_type.h"
#include "lua.h"

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
