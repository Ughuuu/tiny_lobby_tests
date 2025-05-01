local main = {}
-- Load Modules
local api = require("api")

-- Callable Function
main.start_game = api.start_game
main.set_piece = api.set_piece

-- Private Function
main._on_timer_restart_game = api.on_timer_restart_game

-- Callback functions
local callbacks = require("callbacks")
main._can_create = function() return callbacks.can_create(2, 2) end
main._on_create = function() return callbacks.on_create() end
main._on_left = callbacks.on_left
main._on_tags = callbacks.on_tags
main._on_ready = callbacks.on_ready

return main
