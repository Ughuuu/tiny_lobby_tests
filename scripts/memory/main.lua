local main = {}
-- Load Modules
local api = require("api")
local lobby = require("lobby")

-- Callable Functions
main.start_game = api.start_game
main.flip_card = api.flip_card
main.me_command = function (action)
    if type(action) ~= "string" then
        return { error = "Invalid action format." }
    end
    local l = lobby.get()
    local peer_name = l.peers[l.calling_peer_id].user_data["name"]
    lobby.broadcast_chat(string.format("* %s %s", peer_name, action))
    return
end
main.ripple = function (pos_x, pos_y)
    lobby.notify_all({
        type = "ripple",
        pos_x = pos_x,
        pos_y = pos_y
    })
end

-- Private Function
main._on_timer_new_game = api.on_timer_new_game
main._on_timer_end_game = api.on_timer_end_game
main._on_timer_set_new_game_data = api._on_timer_set_new_game_data
main._on_timer_setup_game = api.on_timer_setup_game
main._on_timer_flipped_card = api._on_timer_flipped_card

-- Callback Functions
local callbacks = require("callbacks")
main._can_create_lobby = function() return callbacks.on_create(2, 2) end -- Min 2, max 2 players
main._on_peer_leave = callbacks.on_left
main._can_host_set_tags = callbacks.on_tags
main._can_peer_ready = callbacks.on_ready

return main
