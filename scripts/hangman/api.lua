local lobby = require("lobby")
local NormalGameMode = require("game/normal_game_mode")
local CompetitiveAbunchHangingMode = require("game/competitive_abunch_hanging_mode")
local CompetitiveAllOrNothingMode = require("game/competitive_all_or_nothing_mode")
local CompetitiveLastWrongDiesMode = require("game/competitive_last_wrong_dies_mode")

local function create_game_mode(game_mode_tag)
    if game_mode_tag == "competitive_abunch_hanging" then
        return CompetitiveAbunchHangingMode.new()
    elseif game_mode_tag == "competitive_all_or_nothing" then
        return CompetitiveAllOrNothingMode.new()
    elseif game_mode_tag == "competitive_last_wrong_dies" then
        return CompetitiveLastWrongDiesMode.new()
    else
        return NormalGameMode.new()
    end
end

local api = {}

function api.start_game()
    local l = lobby.get()
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:start_game(l)
end

function api.set_word(word)
    local l = lobby.get()
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:set_word(l, word)
end

function api.guess_word(word)
    local l = lobby.get()
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:guess_word(l, word)
end

function api.guess_letter(letter)
    local l = lobby.get()
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:guess_letter(l, l.calling_peer_id, letter)
end

function api.show_letter_hint()
    local l = lobby.get()
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:show_letter_hint(l, l.calling_peer_id)
end

function api.show_hint()
    local l = lobby.get()
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:show_hint(l, l.calling_peer_id)
end

function api.take_damage()
    local l = lobby.get()
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:take_damage(l, l.calling_peer_id)
end

function api.me_command(action)
    local l = lobby.get()
    local peer_name = l.peers[l.calling_peer_id].user_data["name"]
    lobby.broadcast_chat(string.format("* %s %s", peer_name, action))
    return
end

function api.skip()
    local l = lobby.get()
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:skip(l)
end

function api.end_game(newState)
    local l = lobby.get()
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:end_game(l, newState)
end

function api.check_game_end(peerID, newState)
    local l = lobby.get()
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:check_game_end(l, peerID, newState)
end

function api.on_timer_next_round()
    local l = lobby.get()
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:on_timer_next_round(l)
end

function api.on_timer_restart_game()
    local l = lobby.get()
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:on_timer_restart_game(l)
end

function api.on_timer_word_timeout(dealerID)
    local l = lobby.get()
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:on_timer_word_timeout(l, dealerID)
end

function api.on_timer_guess_timeout()
    local l = lobby.get()
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:on_timer_guess_timeout(l)
end

function api.on_timer_recreate_body()
    local l = lobby.get()
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:on_timer_recreate_body(l)
end

function api.set_initial_data(l)
    local game_mode = create_game_mode(l.tags["game_mode"])
    return game_mode:set_initial_data(l)
end

function api.is_letter(letter)
    local b = letter:byte()
    return b >= string.byte('A') and b <= string.byte('Z')
end

return api
