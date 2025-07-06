-- Load modules
local turn = require("turn")
local api = require("api")
local lobby = require("lobby")

local callbacks = {}

function callbacks.on_create(minPlayers, maxPlayers)
    local l = lobby.get()
    local max_players = l.max_players
    if max_players < minPlayers or max_players > maxPlayers then
        return { error = ("Max players must be between " .. tostring(minPlayers) .. " and " .. tostring(maxPlayers)) }
    end

    l.tags["grid_rows"] = l.tags["grid_rows"] or 3
    l.tags["grid_cols"] = l.tags["grid_cols"] or 8
    if l.tags["grid_rows"] > 10 or l.tags["grid_cols"] > 10 then
        return { error = "Grid size must be less than or equal to 10x10." }
    end
    if l.tags["grid_rows"] < 2 or l.tags["grid_cols"] < 3 then
        return { error = "Grid size must be greater than or equal to 4x6." }
    end
    l.tags["max_points"] = l.tags["max_points"] or 0
    l.tags["max_turns"] = l.tags["max_turns"] or 3
    l.public_data["game_state"] = "setup"
    return
end

function callbacks.on_left()
    local l = lobby.get()
    if l.public_data["game_state"] == "setup" then return end
    
    if not l.peers[l.public_data["dealer"]] then
        api.end_game("lost")
    end
end

function callbacks.on_tags(tags) return turn.validate_game_state_is("setup") end
function callbacks.on_ready(ready) return turn.validate_game_state_is("setup") end

return callbacks
