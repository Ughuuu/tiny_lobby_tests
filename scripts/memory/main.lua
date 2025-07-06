local main = {}
-- Load Modules
local api = require("api")

-- Callable Functions
main.start_game = api.start_game
main.flip_card = api.flip_card
main._on_timer_flipped_card = api._on_timer_flipped_card

-- Private Function
main._on_timer_new_game = api.on_timer_new_game
main._on_timer_end_game = api.on_timer_end_game
main._on_timer_set_new_game_data = api._on_timer_set_new_game_data
main._on_timer_setup_game = api.on_timer_setup_game

-- Callback Functions
local callbacks = require("callbacks")
main._can_create_lobby = function() return callbacks.on_create(2, 2) end -- Min 2, max 2 players
main._on_peer_leave = callbacks.on_left
main._can_host_set_tags = callbacks.on_tags
main._can_peer_ready = callbacks.on_ready

return main
