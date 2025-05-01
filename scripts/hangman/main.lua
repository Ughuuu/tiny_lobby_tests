-- Load Modules
local api = require("api")

local main = {}

-- Callable Function
main.start_game = api.start_game
main.set_word = api.set_word
main.guess_letter = api.guess_letter
main.guess_word = api.guess_word
main.me_command = api.me_command
main.skip = api.skip

-- Private Function
main._on_timer_restart_game = api.on_timer_restart_game
main._on_timer_word_timeout = api.on_timer_word_timeout
main._on_timer_guess_timeout = api.on_timer_guess_timeout

-- Callback functions
local callbacks = require("callbacks")
main._can_create = function() return callbacks.can_create(2, 10) end
main._on_create = function() return callbacks.on_create() end
main._on_tags = callbacks.on_tags
main._on_ready = callbacks.on_ready
main._on_left = callbacks.on_left

return main
