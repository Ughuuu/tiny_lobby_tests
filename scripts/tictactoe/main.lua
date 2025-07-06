local main = {}
local lobby = require("lobby")
-- Load Modules
local api = require("api")

-- Callable Function
main.start_game = api.start_game
main.set_piece = api.set_piece
function main.ripple(pos_x, pos_y)
    lobby.notify_all({
        type = "ripple",
        pos_x = pos_x,
        pos_y = pos_y
    })
end

-- Private Function
main._on_timer_restart_game = api.on_timer_restart_game

-- Callback functions
local callbacks = require("callbacks")
main._can_create_lobby = function() return callbacks.can_create(2, 2) end
main._on_lobby_created = function() return callbacks.on_create() end
main._on_peer_leave = callbacks.on_left
main._can_host_set_tags = callbacks.on_tags
main._can_peer_ready = callbacks.on_ready

return main
