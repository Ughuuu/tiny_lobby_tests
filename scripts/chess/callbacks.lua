-- Load modules
local turn = require("turn")
local lobby = require("lobby")

local callbacks = {}

function callbacks.can_create(minPlayers: number, maxPlayers: number)
    local l = lobby.get()
    local max_players = l.max_players
    if max_players < minPlayers or max_players > maxPlayers then
        return { error = ("Max players must be between " .. tostring(minPlayers) .. " and " .. tostring(maxPlayers)) }
    end
    return
end

function callbacks.on_create()
    local l = lobby.get()
    l.public_data["game_state"] = "setup"
    return
end

function callbacks.on_left()
    local l = lobby.get()
    if l.public_data["game_state"] == "setup" then return end
    
    l.public_data["game_state"] = "setup"
end

function callbacks.on_tags(tags) return turn.validate_game_state_is("setup") end
function callbacks.on_ready(ready) return turn.validate_game_state_is("setup") end

return callbacks
