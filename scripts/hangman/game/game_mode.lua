local GameMode = {}
GameMode.__index = GameMode

function GameMode.new()
    local self = setmetatable({}, GameMode)
    return self
end

-- Abstract methods
function GameMode:start_game(lobby_data) error("start_game not implemented") end
function GameMode:set_word(lobby_data, word) error("set_word not implemented") end
function GameMode:guess_word(lobby_data, word) error("guess_word not implemented") end
function GameMode:guess_letter(lobby_data, peerID, letter) error("guess_letter not implemented") end
function GameMode:show_letter_hint(lobby_data, peerID) error("show_letter_hint not implemented") end
function GameMode:show_hint(lobby_data, peerID) error("show_hint not implemented") end
function GameMode:take_damage(lobby_data, peerID) error("take_damage not implemented") end
function GameMode:skip(lobby_data) error("skip not implemented") end
function GameMode:end_game(lobby_data, newState) error("end_game not implemented") end
function GameMode:check_game_end(lobby_data, peerID, newState) error("check_game_end not implemented") end
function GameMode:on_timer_next_round(lobby_data) error("on_timer_next_round not implemented") end
function GameMode:on_timer_restart_game(lobby_data) error("on_timer_restart_game not implemented") end
function GameMode:on_timer_word_timeout(lobby_data, dealerID) error("on_timer_word_timeout not implemented") end
function GameMode:on_timer_guess_timeout(lobby_data) error("on_timer_guess_timeout not implemented") end
function GameMode:on_timer_recreate_body(lobby_data) error("on_timer_recreate_body not implemented") end
function GameMode:set_initial_data(lobby_data) error("set_initial_data not implemented") end
function GameMode:on_left(lobby_data, peerID) error("on_left not implemented") end
function GameMode:on_join(lobby_data, peerID) error("on_join not implemented") end

return GameMode