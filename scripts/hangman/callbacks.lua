-- Load modules
local turn = require("turn")
local lobby = require("lobby")
local NormalGameMode = require("game/normal_game_mode")
local CompetitiveAbunchHangingMode = require("game/competitive_abunch_hanging_mode")
local CompetitiveAllOrNothingMode = require("game/competitive_all_or_nothing_mode")
local CompetitiveLastWrongDiesMode = require("game/competitive_last_wrong_dies_mode")

-- Game Mode Factory
local function create_game_mode(game_mode_tag)
    if game_mode_tag == "competitive_abunch_hanging" then
        return CompetitiveAbunchHangingMode.new()
    elseif game_mode_tag == "all_or_nothing" then
        return CompetitiveAllOrNothingMode.new()    
    elseif game_mode_tag == "competitive_last_wrong_dies" then
        return CompetitiveLastWrongDiesMode.new()
    else
        return NormalGameMode.new()
    end
end

local callbacks = {}

function callbacks.on_create(minPlayers, maxPlayers)
    local l = lobby.get()
    local max_players = l.max_players
    if max_players < minPlayers or max_players > maxPlayers then
        return { error = ("Max players must be between " .. tostring(minPlayers) .. " and " .. tostring(maxPlayers)) }
    end

    l.tags["max_points"] = l.tags["max_points"] or 0
    l.tags["game_mode"] = l.tags["game_mode"] or "normal"
    l.tags["category"] = l.tags["category"] or "Star Wars"
    l.public_data["game_state"] = "setup"
    return
end

function callbacks.on_left()
    local l = lobby.get()
    if l.public_data["game_state"] == "setup" then return end
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:on_left(l, l.calling_peer_id)
end

function callbacks.on_join()
    local l = lobby.get()
    if l.public_data["game_state"] == "setup" then return end
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:on_join(l, l.calling_peer_id)
end

function callbacks.on_tags(tags)
    local l = lobby.get()
    if tags["game_mode"] ~= l.tags["game_mode"] then
        return { error = "Game mode cannot be changed after the game has started." }
    end
    return turn.validate_game_state_is("setup")
end
function callbacks.on_ready(ready) return turn.validate_game_state_is("setup") end

return callbacks
