-- Load Modules
local api = require("api")

-- Callable Function
start_game = api.start_game
set_word = api.set_word
guess_letter = api.guess_letter
skip = api.skip

-- Private Function
_on_timer_restart_game = api.on_timer_restart_game
_on_timer_word_timeout = api.on_timer_word_timeout

-- Callback functions
local callbacks = require("callbacks")
_on_create = function() return callbacks.on_create(2, 10) end
_on_join = callbacks.on_join
_on_chat = callbacks.on_chat
_on_tags = callbacks.on_tags
_on_kick = callbacks.on_kick
_on_ready = callbacks.on_ready
_on_seal = callbacks.on_seal
_on_left = callbacks.on_left
