local main = {}
-- Load Modules
local api = require("api")

-- Callable Functions
main.start_game = api.start_game
main.move_piece = api.move_piece
main.rotate_piece = api.rotate_piece
main.hold_piece = api.hold_piece
main.hard_drop = api.hard_drop

-- Private Functions
main._on_timer_restart_game = api.on_timer_restart_game

-- Callback functions
local callbacks = require("callbacks")
main._on_create = function() return callbacks.on_create(1, 99) end
main._on_left = callbacks.on_left
main._on_tags = callbacks.on_tags
main._on_ready = callbacks.on_ready
main._on_tick = callbacks.on_tick

return main
