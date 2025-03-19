-- Load Modules
local api = require("api")

-- Callable Function
start_game = api.start_game
place_piece = api.place_piece

-- Private Function
_on_timer_restart_game = api.on_timer_restart_game

-- Callback functions
local callbacks = require("callbacks")
_on_create = function(peerID) return callbacks.on_create(peerID, 2, 10) end
_on_join = callbacks.on_join
_on_chat = callbacks.on_chat
_on_tags = callbacks.on_tags
_on_kick = callbacks.on_kick
_on_ready = callbacks.on_ready
_on_seal = callbacks.on_seal
_on_left = callbacks.on_left
