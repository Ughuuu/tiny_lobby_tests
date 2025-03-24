-- Load modules
local turn = require("turn")
local api = require("api")
local helper = require("helper")
local lobby = require("lobby")

local callbacks = {}

function callbacks.on_create(minPlayers, maxPlayers)
    local l = lobby.get()
    local max_players = l.max_players
    if max_players < minPlayers or max_players > maxPlayers then
        return { error = ("Max players must be between " .. tostring(minPlayers) .. " and " .. tostring(maxPlayers)) }
    end

    l.tags["max_points"] = l.tags["max_points"] or 0
    l.public_data["game_state"] = "setup"
    return nil
end

function callbacks.on_left() return nil end
function callbacks.on_join() return nil end
function callbacks.on_chat(message) return nil end
function callbacks.on_tags(tags) return turn.validate_game_state_is("setup") end
function callbacks.on_kick() return nil end
function callbacks.on_ready(ready) return turn.validate_game_state_is("setup") end
function callbacks.on_seal(seal) return nil end

return callbacks