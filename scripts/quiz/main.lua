local main = {}
-- Load Modules
local api = require("api")

-- Callable Function
main.start_game = api.start_game
main.guess_answer = api.guess_answer
main.me_command = api.me_command

-- Private Function
main._on_timer_question_expired = api.on_timer_question_expired
main._on_timer_question_reset = api.on_timer_question_reset
main._on_timer_restart_game = api.on_timer_restart_game

-- Callback functions
local callbacks = require("callbacks")
main._can_create = function() return callbacks.can_create(2, 10) end
main._on_create = function() return callbacks.on_create(2, 10) end
main._on_tags = callbacks.on_tags
main._on_ready = callbacks.on_ready
main._on_join = callbacks.on_join

return main
