-- Load Modules
local api = require("api")
local lobby = require("lobby")

local main = {}

-- Callable Function
main.start_game = api.start_game
main.set_word = api.set_word
main.guess_letter = api.guess_letter
main.guess_word = api.guess_word
main.me_command = function (action)
    if type(action) ~= "string" then
        return { error = "Invalid action format." }
    end
    local l = lobby.get()
    local peer_name = l.peers[l.calling_peer_id].user_data["name"]
    l.broadcast_chat(string.format("* %s %s", peer_name, action))
    return
end
main.skip = api.skip
main.show_hint = api.show_hint
main.show_letter_hint = api.show_letter_hint
main.ripple = function (pos_x, pos_y)
    lobby.get().notify_all({
        type = "ripple",
        pos_x = pos_x,
        pos_y = pos_y
    })
end

-- Private Function
main._on_timer_restart_game = api.on_timer_restart_game
main._on_timer_word_timeout = api.on_timer_word_timeout
main._on_timer_guess_timeout = api.on_timer_guess_timeout
main._on_timer_next_round = api.on_timer_next_round
main._on_timer_next_word = api.on_timer_next_word
main._on_timer_kick_peer = api.on_timer_kick_peer
main._on_timer_broadcast_remaining_time = api.on_timer_broadcast_remaining_time
main._on_timer_recreate_body = api.on_timer_recreate_body
main._on_timer_start_game = api.on_timer_start_game

-- Callback functions
local callbacks = require("callbacks")
main._can_create_lobby = function() return callbacks.on_create(2, 10) end
main._can_host_set_tags = callbacks.on_tags
main._can_peer_ready = callbacks.on_ready
main._on_peer_leave = callbacks.on_left
main._on_peer_joined = callbacks.on_join
main._on_lobby_tick = callbacks.on_tick
main._on_server_init = callbacks.on_init
main._on_server_reload = callbacks.on_reload

return main
