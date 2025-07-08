local main = {}
-- Load Modules
local api = require("api")
local lobby = require("lobby")

-- Callable Function
main.start_game = api.start_game
main.guess_answer = api.guess_answer
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
main._on_timer_question_expired = api.on_timer_question_expired
main._on_timer_question_reset = api.on_timer_question_reset
main._on_timer_restart_game = api.on_timer_restart_game

-- Callback functions
local callbacks = require("callbacks")
main._can_create_lobby = function() return callbacks.can_create(2, 10) end
main._on_lobby_created = function() return callbacks.on_create(2, 10) end
main._can_host_set_tags = callbacks.on_tags
main._can_peer_ready = callbacks.on_ready
main._on_peer_joined = callbacks.on_join

return main
