local turn = require("turn")
local lobby = require("lobby")
local system = require("system")
local GameMode = require("game/game_mode")

local NormalGameMode = setmetatable({}, { __index = GameMode })
NormalGameMode.__index = NormalGameMode

function NormalGameMode.new()
    local self = setmetatable(GameMode.new(), NormalGameMode)
    return self
end

function NormalGameMode:start_game(lobby_data)
    if lobby_data.peers[lobby_data.calling_peer_id].id ~= lobby_data.host then
        return { error = "You are not the host" }
    end
    if lobby_data.public_data["game_state"] ~= "setup" then
        return { error = "Game already started" }
    end
    for k, _ in pairs(lobby_data.peers) do
        lobby_data.peers[k].public_data["total_points"] = 0
    end
    lobby_data.private_data["words"] = {}
    return self:set_initial_data(lobby_data)
end

function NormalGameMode:set_word(lobby_data, word)
    word = string.upper(tostring(word))
    if #word > 20 then
        return { error = "Word too long." }
    end

    local foundLetter = false
    for i = 1, #word do
        local letter = word:sub(i, i)
        if not (self:is_letter(letter) or letter == ' ') then
            return { error = "Invalid word." }
        end
        if self:is_letter(letter) then foundLetter = true end
    end
    if not foundLetter then
        return { error = "Invalid word." }
    end

    local err = turn.validate_game_state_is("setting_word")
    if err then return err end

    local dealerID = lobby_data.public_data["dealer"]
    if dealerID ~= lobby_data.calling_peer_id then return { error = "Only the dealer can set the word." } end
    if lobby_data.private_data["words"][word] then return { error = "Word was already used." } end

    lobby_data.peers[dealerID].private_data["word"] = word
    local words = lobby_data.private_data["words"]
    words[word] = true
    lobby_data.private_data["words"] = words
    lobby_data.public_data["turn_timestamp"] = system.get_time_since_epoch()
    lobby_data.public_data["game_state"] = "playing"
    lobby_data.public_data["guessed"] = ""

    for i = 1, #word do
        local letter = word:sub(i, i)
        lobby_data.public_data["guessed"] = lobby_data.public_data["guessed"] .. (letter == ' ' and ' ' or '_')
    end
    return
end

function NormalGameMode:guess_word(lobby_data, word)
    local err = turn.validate_game_state_is("playing")
    if err then return err end

    local dealerID = lobby_data.public_data["dealer"]
    if dealerID == lobby_data.calling_peer_id then
        return { error = "The dealer cannot guess the word." }
    end
    if #word == 0 then return { error = "Empty word." } end
    if #word > 20 then return { error = "Too many letters." } end
    if #word ~= #lobby_data.peers[dealerID].private_data["word"] then return { error = "Incorrect length." } end
    for i = 1, #word do
        local letter = word:sub(i, i)
        if not (self:is_letter(letter) or letter == ' ') then
            return { error = "Only alphabetic characters and spaces are allowed." }
        end
    end
    if lobby_data.private_data["tried_words"][word] then
        return { error = "Word was already tried." }
    end

    local words = lobby_data.private_data["tried_words"]
    words[word] = true
    lobby_data.private_data["tried_words"] = words

    for i = 1, #word do
        local letter = word:sub(i, i)
        if letter ~= ' ' then
            local val = self:guess_letter(lobby_data, lobby_data.calling_peer_id, letter)
            if val then return val end
        end
    end
    return
end

function NormalGameMode:guess_letter(lobby_data, peerID, letter)
    letter = string.upper(tostring(letter))
    if #letter ~= 1 then return { error = "Too many letters." } end
    if not self:is_letter(letter) then return { error = "Invalid letter." } end
    local err = turn.validate_game_state_is("playing")
    if err then return err end

    local dealerID = lobby_data.public_data["dealer"]
    if dealerID == peerID then return { error = "The dealer cannot guess the word." } end
    if lobby_data.public_data["pressed"][letter] then
        return { error = "Letter was already pressed." }
    end

    local pressed = lobby_data.public_data["pressed"]
    pressed[letter] = peerID
    lobby_data.public_data["pressed"] = pressed

    local peer_name = string.upper(tostring(lobby_data.peers[peerID].user_data["name"]))
    local word = lobby_data.peers[dealerID].private_data["word"]
    if not string.find(word, letter, 1, true) then
        lobby_data.public_data["health"] = lobby_data.public_data["health"] - 1
        if lobby_data.public_data["health"] <= 0 then self:end_game(lobby_data, "lost") end
        lobby.broadcast_chat(string.format("%s guessed the wrong letter, %s", peer_name, letter))
        return { error = "Letter is not in the word." }
    end

    local points = 0
    local guessed = lobby_data.public_data["guessed"]
    for i = 1, #word do
        if word:sub(i, i) == letter then
            points = points + 1
            guessed = guessed:sub(1, i - 1) .. letter .. guessed:sub(i + 1)
        end
    end
    lobby_data.public_data["guessed"] = guessed

    local total_points = (lobby_data.peers[peerID].public_data["total_points"] or 0) + points
    lobby_data.peers[peerID].public_data["total_points"] = total_points
    lobby.broadcast_chat(string.format(
        "%s guessed letter %s. Gained %d points. Total: %d",
        peer_name, letter, points, total_points
    ))

    if lobby_data.public_data["guessed"] == word then
        lobby.start_timer("_on_timer_restart_game", 1)
        self:end_game(lobby_data, "won")
    end
    return
end

function NormalGameMode:show_letter_hint(lobby_data, peerID)
    return { error = "Letter hints are not supported in normal mode." }
end

function NormalGameMode:show_hint(lobby_data, peerID)
    return { error = "Hints are not supported in normal mode." }
end

function NormalGameMode:take_damage(lobby_data, peerID)
    lobby_data.public_data["health"] = lobby_data.public_data["health"] - 1
    if lobby_data.public_data["health"] <= 0 then
        self:end_game(lobby_data, "lost")
    end
    lobby_data.peers[peerID].private_data["timer"] = system.get_time_since_epoch()
    return
end

function NormalGameMode:skip(lobby_data)
    if lobby_data.public_data["dealer"] ~= lobby_data.calling_peer_id then return { error = "Only dealer can skip." } end
    if lobby_data.public_data["game_state"] ~= "setting_word" then return { error = "Word is already set." } end
    self:end_game(lobby_data, "lost")
    return
end

function NormalGameMode:end_game(lobby_data, newState)
    local points = 0
    for i = 1, #lobby_data.public_data["guessed"] do
        if lobby_data.public_data["guessed"]:sub(i, i) == '_' then points = points + 1 end
    end

    lobby_data.public_data["game_state"] = newState
    local dealerID = lobby_data.public_data["dealer"]
    if lobby_data.peers[dealerID] then
        lobby_data.public_data["guessed"] = lobby_data.peers[dealerID].private_data["word"]
        local total_points = (lobby_data.peers[dealerID].public_data["total_points"] or 0) + points
        lobby_data.peers[dealerID].public_data["total_points"] = total_points
    end

    if newState == "won" then
        lobby.broadcast_chat("The guessers successfully found the word!")
    elseif newState == "lost" then
        local dealer_name = lobby_data.peers[dealerID].user_data["name"] or ""
        local total = lobby_data.peers[dealerID].public_data["total_points"]
        lobby.broadcast_chat(string.format("%s won! Gained %d points. Total: %d", dealer_name, points, total))
    end

    lobby.start_timer("_on_timer_restart_game", 1)
end

function NormalGameMode:check_game_end(lobby_data, peerID, newState)
    -- Normal mode doesn't use per-player win/loss states
end

function NormalGameMode:on_timer_next_round(lobby_data)
    -- Normal mode doesn't use next round logic
end

function NormalGameMode:on_timer_restart_game(lobby_data)
    local game_ended = false
    for k, _ in pairs(lobby_data.peers) do
        lobby_data.peers[k].private_data["word"] = nil
        if lobby_data.peers[k].public_data["total_points"] >= lobby_data.tags["max_points"] and lobby_data.tags["max_points"] ~= 0 then
            game_ended = true
        end
    end
    if game_ended then
        lobby_data.public_data["game_state"] = "setup"
        return
    end
    return self:set_initial_data(lobby_data)
end

function NormalGameMode:on_timer_word_timeout(lobby_data, dealerID)
    if lobby_data.public_data["game_state"] == "setting_word" and lobby_data.public_data["dealer"] == dealerID then
        self:end_game(lobby_data, "lost")
    end
end

function NormalGameMode:on_timer_guess_timeout(lobby_data)
    -- Normal mode doesn't use guess timeout
end

function NormalGameMode:set_initial_data(lobby_data)
    lobby_data.public_data["turn_timestamp"] = system.get_time_since_epoch()
    lobby_data.public_data["game_state"] = "setting_word"
    lobby_data.public_data["health"] = 6
    lobby_data.public_data["guessed"] = ""
    lobby_data.public_data["pressed"] = {}
    lobby_data.private_data["tried_words"] = {}
    if lobby_data.public_data["dealer"] and not lobby_data.peers[lobby_data.public_data["dealer"]] then
        lobby_data = turn.increment_dealer(lobby_data, -1)
    end
    lobby_data = turn.increment_dealer(lobby_data, 1)
    lobby_data = turn.increment_turn(lobby_data, 1)
    lobby.start_timer("_on_timer_word_timeout", 180, lobby_data.public_data["dealer"])
    return lobby_data
end

function NormalGameMode:on_tick(l, tickrate)
    local current_time = system.get_time_since_epoch()
    for _, peer in pairs(l.peers) do
        if current_time - peer.private_data["timer"] >= 10000 then
            print(string.format("Peer %s timed out. Taking damage.", peer.user_data["name"]))
            self:take_damage(l, peer.id)
        end
    end
end

function NormalGameMode:on_left(lobby_data, peerID)
    -- if lobby_data.peers[peerID].id == lobby_data.public_data["dealer"] then
    --     self:end_game(lobby_data, "lost")
    -- end
    return
end

function NormalGameMode:is_letter(letter)
    local b = letter:byte()
    return b >= string.byte('A') and b <= string.byte('Z')
end

return NormalGameMode