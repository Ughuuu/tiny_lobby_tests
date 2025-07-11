local main = {}
local lobby = require("lobby")
-- Load Modules
local api = require("api")
local turn = require("turn")

-- Callable Function
main.start_game = api.start_game
main.set_piece = api.set_piece
main.me_command = function (action)
    if type(action) ~= "string" then
        return { error = "Invalid action format." }
    end
    local l = lobby.get()
    local peer_name = l.peers[l.calling_peer_id].user_data["name"]
    l.broadcast_chat(string.format("* %s %s", peer_name, action))
    return
end
main.ripple = function (pos_x, pos_y)
    lobby.get().notify_all({
        type = "ripple",
        pos_x = pos_x,
        pos_y = pos_y
    })
end

-- Private Function
main._on_timer_restart_game = api.on_timer_restart_game

-- Callback functions
main._can_create_lobby = function()
    -- Only allow creation if max_peers is 2
    local l = lobby.get()
    local max_players = l.max_players
    if max_players < 2 or max_players > 2 then
        return { error = ("Max players must be between " .. tostring(2) .. " and " .. tostring(2)) }
    end
    if l.tags["game_mode"] ~= "normal_mode" and l.tags["game_mode"] ~= "dissapearing_mode" then
        l.tags["game_mode"] = "normal_mode"
    end
    return
end
main._on_lobby_created = function()
    local l = lobby.get()
    l.tags["max_points"] = l.tags["max_points"] or 0
    -- Set initial game state
    l.public_data["game_state"] = "setup"
    return
end
main._on_peer_leave = function()
    local l = lobby.get()
    -- If a player leaves, reset the game state to setup
    l.public_data["game_state"] = "setup"
end
main._can_peer_ready = function()
    return turn.validate_game_state_is("setup")
end
main._can_host_set_tags = function(tags)
    return turn.validate_game_state_is("setup")
end
main._can_host_seal = function(seal)
    return turn.validate_game_state_is("setup")
end
main._can_host_resize = function()
    return { error = "Cannot resize." }
end

return main
