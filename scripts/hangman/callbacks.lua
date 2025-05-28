-- Load modules
local turn = require("turn")
local lobby = require("lobby")
local NormalGameMode = require("game/normal_game_mode")
local CompetitiveAbunchHangingMode = require("game/competitive_abunch_hanging_mode")
local NormalAbunchHangingMode = require("game/normal_abunch_hanging_mode")
local NormalAllOrNothingMode = require("game/normal_all_or_nothing_mode")
local NormalLastWrongDiesMode = require("game/normal_last_wrong_dies_mode")

-- Game Mode Factory
local function create_game_mode(game_mode_tag)    
    if game_mode_tag == "competitive_abunch_hanging" then
        return CompetitiveAbunchHangingMode.new()
    elseif game_mode_tag == "normal_abunch_hanging" then
        return NormalAbunchHangingMode.new()
    elseif game_mode_tag == "normal_all_or_nothing" then
        return NormalAllOrNothingMode.new()    
    elseif game_mode_tag == "normal_last_wrong_dies" then
        return NormalLastWrongDiesMode.new()
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
    local game_mode = l.tags["game_mode"]
    if game_mode == "competitive_abunch_hanging" then
        l.host = ""
        l.public_data["game_state"] = "setup"
        l.tags["max_points"] = 0
        l.tags["category"] = "Geography"
        l.max_players = 6
        l.name = "Competitive"
        return
    end
    l.tags["max_points"] = l.tags["max_points"] or 0
    l.tags["game_mode"] = l.tags["game_mode"] or "normal_mode"
    l.tags["category"] = l.tags["category"] or "Geography"
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

function callbacks.on_tick(tickrate)
    local l = lobby.get()
    if l.public_data["game_state"] == "setup" then return end
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:on_tick(l, tickrate)
end

function callbacks.on_tags(tags)
    local l = lobby.get()
    if tags["game_mode"] ~= l.tags["game_mode"] then
        return { error = "Game mode cannot be changed after the game has started." }
    end
    return turn.validate_game_state_is("setup")
end

function callbacks.on_ready(ready) 
    local l = lobby.get()
    if l.tags["game_mode"] == "competitive_abunch_hanging" and ready == true and l.peers_count >= 2 then
        local game_mode = create_game_mode(l.tags["game_mode"])
        return game_mode:on_ready(l)
    end
    return
end

return callbacks
