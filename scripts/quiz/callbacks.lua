-- Load modules
local turn = require("turn")
local lobby = require("lobby")

local callbacks = {}

function callbacks.on_create(minPlayers, maxPlayers)
    local l = lobby.get()
    local max_players = l.max_players
    if max_players < minPlayers or max_players > maxPlayers then
        return { error = ("Max players must be between " .. tostring(minPlayers) .. " and " .. tostring(maxPlayers)) }
    end

    l.tags["max_points"] = l.tags["max_points"] or 0
    l.tags["category"] = l.tags["category"] or "Trivia"
    l.public_data["game_state"] = "setup"
    return
end

function callbacks.on_tags(tags) return turn.validate_game_state_is("setup") end
function callbacks.on_ready(ready) return turn.validate_game_state_is("setup") end

return callbacks
