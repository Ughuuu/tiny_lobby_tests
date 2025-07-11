local turn = require("turn")
local helper = require("helper")
local lobby = require("lobby")
local system = require("system")
local GameMode = require("game/game_mode")

local NormalGameMode = setmetatable({}, { __index = GameMode })
NormalGameMode.__index = NormalGameMode

local WORD_TIMEOUT = 180 -- seconds
-- local WORD_TIMEOUT = 10 -- seconds, for testing purposes

function NormalGameMode.new()
    local self = setmetatable(GameMode.new(), NormalGameMode)
    return self
end

function NormalGameMode:start_game(l)
    if l.calling_peer_id ~= l.host then
        return { error = "You are not the host" }
    end
    if l.public_data["game_state"] ~= "setup" then
        return { error = "Game already started" }
    end
    for k, _ in pairs(l.peers) do
        l.peers[k].public_data["total_points"] = 0
    end
    l.private_data["words"] = {}
    return self:set_initial_data(l)
end

function NormalGameMode:set_word(l, word)
    if #word > 20 then
        return { error = "Word too long." }
    end

    local foundLetter = false
    local word_str = ""
    for _, letter in ipairs(word) do
        word_str = word_str .. letter
        if not (helper.is_letter(letter, l.tags["lang"]) or letter == ' ') then
            return { error = "Invalid word." }
        end
        if helper.is_letter(letter, l.tags["lang"]) then foundLetter = true end
    end
    if not foundLetter then
        return { error = "Invalid word." }
    end

    local err = turn.validate_game_state_is("setting_word")
    if err then return err end

    local dealerID = l.public_data["dealer"]
    if dealerID ~= l.calling_peer_id then return { error = "Only the dealer can set the word." } end
    if l.private_data["words"][word_str] then return { error = "Word was already used." } end

    l.peers[dealerID].private_data["word"] = word
    local words = l.private_data["words"]
    words[word_str] = true
    l.private_data["words"] = words
    l.public_data["turn_timestamp"] = system.get_time_since_epoch()
    l.public_data["game_state"] = "playing"
    local guessed = {}
    for i, letter in ipairs(word) do
        if letter == " " then
            table.insert(guessed, " ")
        else
            table.insert(guessed, "_")
        end
    end
    l.public_data["guessed"] = guessed
    l.start_timer("_on_timer_guess_timeout", WORD_TIMEOUT)
    return
end

function NormalGameMode:guess_word(l, word)
    local err = turn.validate_game_state_is("playing")
    if err then return err end

    local dealerID = l.public_data["dealer"]
    if dealerID == l.calling_peer_id then
        return { error = "The dealer cannot guess the word." }
    end
    if #word == 0 then return { error = "Empty word." } end
    if #word > 20 then return { error = "Too many letters." } end
    if #word ~= #l.peers[dealerID].private_data["word"] then return { error = "Incorrect length." } end
    
    local word_str = ""
    for _, letter in ipairs(word) do
        word_str = word_str .. letter
        if not (helper.is_letter(letter, l.tags["lang"]) or letter == ' ') then
            return { error = "Only alphabetic characters and spaces are allowed." }
        end
    end
    if l.private_data["tried_words"][word_str] then
        return { error = "Word was already tried." }
    end

    local words = l.private_data["tried_words"]
    words[word_str] = true
    l.private_data["tried_words"] = words

    for _, letter in ipairs(word) do
        if letter ~= ' ' then
            local val = self:guess_letter(l, l.calling_peer_id, letter)
            if val then return val end
        end
    end
    return
end

function NormalGameMode:guess_letter(l, peerID, letter)
    if not helper.is_letter(letter, l.tags["lang"]) then return { error = "Invalid letter." } end
    local err = turn.validate_game_state_is("playing")
    if err then return err end

    local dealerID = l.public_data["dealer"]
    if dealerID == peerID then return { error = "The dealer cannot guess the word." } end
    if l.public_data["pressed"][letter] then
        return { error = "Letter was already pressed." }
    end

    local pressed = l.public_data["pressed"]
    pressed[letter] = peerID
    l.public_data["pressed"] = pressed

    local peer_name = l.peers[peerID].user_data["name"]
    local word = l.peers[dealerID].private_data["word"]
    if not helper.array_contains(word, letter) then
        l.public_data["health"] = l.public_data["health"] - 1
        if l.public_data["health"] <= 0 then self:end_game(l, "lost") end
        l.broadcast_chat(string.format("%s guessed the wrong letter, %s", peer_name, letter))
        return { error = "Letter is not in the word." }
    end

    local points = 0
    local guessed = l.public_data["guessed"]
    for i, letter_word in ipairs(word) do
        if letter_word == letter then
            points = points + 1
            guessed[i] = letter
        end
    end
    l.public_data["guessed"] = guessed

    local total_points = (l.peers[peerID].public_data["total_points"] or 0) + points
    l.peers[peerID].public_data["total_points"] = total_points
    l.broadcast_chat(string.format(
        "%s guessed letter %s. Gained %d points. Total: %d",
        peer_name, letter, points, total_points
    ))

    if helper.arrays_equal(l.public_data["guessed"], word) then
        l.start_timer("_on_timer_restart_game", 1)
        self:end_game(l, "won")
    end
    return
end

function NormalGameMode:show_letter_hint(l, peerID)
    return { error = "Letter hints are not supported in normal mode." }
end

function NormalGameMode:show_hint(l, peerID)
    return { error = "Hints are not supported in normal mode." }
end

function NormalGameMode:take_damage(l, peerID)
    l.public_data["health"] = l.public_data["health"] - 1
    if l.public_data["health"] <= 0 then
        self:end_game(l, "lost")
    end
    return
end

function NormalGameMode:skip(l)
    if l.public_data["dealer"] ~= l.calling_peer_id then return { error = "Only dealer can skip." } end
    if l.public_data["game_state"] ~= "setting_word" then return { error = "Word is already set." } end
    self:end_game(l, "lost")
    return
end

function NormalGameMode:end_game(l, newState)
    local points = 0
    for _, letter in ipairs(l.public_data["guessed"]) do
        if letter == '_' then points = points + 1 end
    end

    l.public_data["game_state"] = newState
    local dealerID = l.public_data["dealer"]
    if l.peers[dealerID] then
        l.public_data["guessed"] = l.peers[dealerID].private_data["word"]
        local total_points = (l.peers[dealerID].public_data["total_points"] or 0) + points
        l.peers[dealerID].public_data["total_points"] = total_points
    end

    if newState == "won" then
        l.broadcast_chat("The guessers successfully found the word!")
    elseif newState == "lost" then
        local dealer_name = l.peers[dealerID].user_data["name"] or ""
        local total = l.peers[dealerID].public_data["total_points"]
        l.broadcast_chat(string.format("%s won! Gained %d points. Total: %d", dealer_name, points, total))
    end

    l.start_timer("_on_timer_restart_game", 1)
end

function NormalGameMode:check_game_end(l, peerID, newState)
    -- Normal mode doesn't use per-player win/loss states
end

function NormalGameMode:on_timer_next_round(l)
    -- Normal mode doesn't use next round logic
end

function NormalGameMode:on_timer_restart_game(l)
    -- Timer to restart the game so it doesn't end immediately
    local game_ended = false
    for k, _ in pairs(l.peers) do
        l.peers[k].private_data["word"] = nil
        if l.peers[k].public_data["total_points"] >= l.tags["max_points"] and l.tags["max_points"] ~= 0 then
            game_ended = true
        end
    end
    if game_ended or helper.peers_length(l) <= 1 then
        l.public_data["game_state"] = "setup"
        return
    end
    return self:set_initial_data(l)
end

function NormalGameMode:on_timer_word_timeout(l, dealerID)
    -- If same dealer and still setting word, pass turn
    if l.public_data["game_state"] == "setting_word" and l.public_data["dealer"] == dealerID then
        self:end_game(l, "lost")
    end
end

function NormalGameMode:on_timer_guess_timeout(l)
    if l.public_data["game_state"] ~= "playing" then
        return { error = "Guess timeout only applies during the game." }
    end
    l.public_data["turn_timestamp"] = system.get_time_since_epoch()
    self:end_game(l, "lost")
    return
end

function NormalGameMode:set_initial_data(l)
    l.public_data["turn_timestamp"] = system.get_time_since_epoch()
    l.public_data["game_state"] = "setting_word"
    l.public_data["health"] = 6
    l.public_data["guessed"] = {}
    l.public_data["pressed"] = {}
    l.private_data["tried_words"] = {}
    if l.public_data["dealer"] and not l.peers[l.public_data["dealer"]] then
        l = turn.increment_dealer(l, -1)
    end
    l = turn.increment_dealer(l, 1)
    l = turn.increment_turn(l, 1)
    l.start_timer("_on_timer_word_timeout", WORD_TIMEOUT, l.public_data["dealer"])
    return l
end

function NormalGameMode:on_tick(l, tickrate)
    -- Normal mode doesn't use tick logic
    return
end

function NormalGameMode:on_left(l, peerID)
    -- If the dealer leaves, pass his turn
    -- If there is 1 more player left and he hasn't set word, end game
    if l.peers[peerID].id == l.public_data["dealer"] or (helper.peers_length(l) <= 2) then
        self:end_game(l, "lost")
    end
    return
end

return NormalGameMode
