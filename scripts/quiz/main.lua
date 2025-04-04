local main = {}
-- Load Modules
local api = require("api")

-- Callable Function
main.start_game = api.start_game
main.guess_answer = api.guess_answer

-- Private Function
main._on_timer_question_expired = api.on_timer_question_expired
main._on_timer_question_reset = api.on_timer_question_reset
main._on_timer_restart_game = api.on_timer_restart_game

-- Callback functions
local callbacks = require("callbacks")
main._on_create = function() return callbacks.on_create(2, 10) end
main._on_join = callbacks.on_join
main._on_chat = callbacks.on_chat
main._on_tags = callbacks.on_tags
main._on_kick = callbacks.on_kick
main._on_ready = callbacks.on_ready
main._on_seal = callbacks.on_seal
main._on_left = callbacks.on_left

return main
