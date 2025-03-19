local turn = require("turn")
local helper = require("helper")

local api = {}

function api.start_game(peerID)
    local l = lobby
    local ord = helper.peers_ordered(l)
    if l.peers[peerID].id ~= l.host then
        return { error = "You are not the host" }
    end
    if l.public_data["game_state"] ~= "setup" then
        return { error = "Game already started" }
    end
    for k, _ in pairs(l.peers) do
        l.peers[k].public_data["points"] = 0
        l.peers[k].public_data["total_points"] = 0
    end
    l.private_data["words"] = {}
    l = api.set_initial_data(l)
end

function api.set_word(peerID, word)
    word = string.upper(tostring(word))
    if #word > 20 then
        return { error = "Too long word." }
    end

    local foundLetter = false
    for i = 1, #word do
        local letter = word:sub(i, i)
        if not (api.is_letter(letter) or letter == ' ') then
            return { error = "Invalid word." }
        end
        if api.is_letter(letter) then foundLetter = true end
    end
    if not foundLetter then
        return { error = "Invalid word." }
    end

    local l = lobby
    local err = turn.validate_game_state_is("setting_word")
    if err then return err end

    local dealerID = l.public_data["dealer"]
    if dealerID ~= peerID then return "Only the dealer can set the word." end
    if l.private_data["words"][word] then return "Word was already used." end

    l.peers[dealerID].private_data["word"] = word
    l.private_data["words"][word] = true
    l.public_data["game_state"] = "playing"
    l.public_data["guessed"] = ""

    for i = 1, #word do
        local letter = word:sub(i, i)
        l.public_data["guessed"] = l.public_data["guessed"] .. (letter == ' ' and ' ' or '_')
    end
end

function api.guess_letter(peerID, letter)
    letter = string.upper(tostring(letter))
    if #letter ~= 1 then return { error = "Too many letters." } end

    if not api.is_letter(letter) then return { error = "Invalid letter." } end

    local l = lobby
    local dealerID = l.public_data["dealer"]
    if dealerID == peerID then return { error = "The dealer cannot guess the word." } end

    local err = turn.validate_game_state_is("playing")
    if err then return err end

    if l.public_data["pressed"][letter] then
        return { error = "Letter was already pressed." }
    end

    local word = l.peers[dealerID].private_data["word"]
    if not string.find(word, letter, 1, true) then
        l.public_data["health"] = l.public_data["health"] - 1
        l.public_data["pressed"][letter] = peerID
        if l.public_data["health"] == 0 then api.end_game("lost") end
        return { error = "Letter is not in the word." }
    end

    local points = 0
    local guessed = l.public_data["guessed"]
    for i = 1, #word do
        if word:sub(i, i) == letter then
            points = points + 1
            guessed = guessed:sub(1, i - 1) .. letter .. guessed:sub(i + 1)
        end
    end
    l.public_data["guessed"] = guessed

    for k, _ in pairs(l.peers) do
        l.peers[k].public_data["points"] = 0
    end

    l.peers[peerID].public_data["points"] = points
    l.peers[peerID].public_data["total_points"] = (l.peers[peerID].public_data["total_points"] or 0) + points
    l.public_data["pressed"][letter] = peerID

    if l.public_data["guessed"] == word then
        start_timer("_on_timer_restart_game", 1)
        api.end_game("won")
    end
end

function api.skip(peerID)
    local l = lobby
    if l.public_data["dealer"] ~= peerID then return { error = "Only dealer can skip." } end
    if l.public_data["game_state"] ~= "setting_word" then return { error = "Word is already set." } end
    api.end_game("lost")
end

function api.end_game(newState)
    local l = lobby
    local points = 0
    for i = 1, #l.public_data["guessed"] do
        if l.public_data["guessed"]:sub(i, i) == '_' then points = points + 1 end
    end
    l.public_data["game_state"] = newState
    local dealerID = l.public_data["dealer"]
    if l.peers[dealerID] then
        l.public_data["guessed"] = l.peers[dealerID].private_data["word"]
        l.peers[dealerID].public_data["points"] = points
    end
    start_timer("_on_timer_restart_game", 1)
end

function api.on_timer_restart_game()
    local l = lobby
    local game_ended = false
    for k, _ in pairs(l.peers) do
        l.peers[k].private_data["word"] = nil
        if l.peers[k].public_data["total_points"] >= l.tags["max_points"] and l.tags["max_points"] ~= 0 then
            game_ended = true
        end
    end
    if game_ended then
        l.public_data["game_state"] = "setup"
        return
    end
    l = api.set_initial_data(l)
end

function api.set_initial_data(l)
    l.public_data["game_state"] = "setting_word"
    l.public_data["health"] = 6
    l.public_data["guessed"] = ""
    l.public_data["pressed"] = {}
    l = turn.increment_dealer(l)
    l = turn.increment_turn(l)
    for k, _ in pairs(l.peers) do
        l.peers[k].public_data["points"] = 0
    end
    start_timer("_on_timer_word_timeout", 180, l.public_data["dealer"])
    return l
end

function api.on_timer_word_timeout(dealerID)
    local l = lobby
    if l.public_data["game_state"] == "setting_word" or l.public_data["dealer"] == dealerID then
        api.end_game("lost")
    end
end

function api.is_letter(letter)
    local b = letter:byte()
    return b >= string.byte('A') and b <= string.byte('Z')
end

return api
