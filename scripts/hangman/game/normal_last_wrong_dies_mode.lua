local turn = require("turn")
local lobby = require("lobby")
local system = require("system")
local GameMode = require("game/game_mode")

local LastWrongDies = setmetatable({}, { __index = GameMode })
LastWrongDies.__index = LastWrongDies

function LastWrongDies.new()
    local self = setmetatable(GameMode.new(), LastWrongDies)
    return self
end

function LastWrongDies:set_points(l, peer, points)
    local player_points = l.private_data["players_points"]
    player_points[peer.platform .. ":" .. peer.platform_id] = points
    l.private_data["players_points"] = player_points
    peer.public_data["total_points"] = points
end

function LastWrongDies:set_state(l, peer, state)
    local player_states = l.private_data["players_states"]
    player_states[peer.platform .. ":" .. peer.platform_id] = state
    l.private_data["players_states"] = player_states
    peer.public_data["state"] = state
end

function LastWrongDies:start_game(l)
    if l.peers[l.calling_peer_id].id ~= l.host then
        return { error = "You are not the host" }
    end
    if l.public_data["game_state"] ~= "setup" then
        return { error = "Game already started" }
    end
    local l_new, word, hint = self:get_competitive_word(l)
    l = l_new
    l.public_data["players_guessed"] = {}
    l.private_data["players_states"] = {}
    l.private_data["players_points"] = {}
    for k, _ in pairs(l.peers) do
        local player_guessed = l.public_data["players_guessed"]
        player_guessed[k] = false
        l.public_data["players_guessed"] = player_guessed
        self:set_points(l, l.peers[k], 0)
        self:set_state(l, l.peers[k], "alive")
    end
    l.private_data["word"] = word
    l.public_data["hint"] = hint
    l.public_data["health"] = 6
    l.public_data["announcement_message"] = ""
    lobby.start_timer("_on_timer_guess_timeout", 10)
    return self:set_initial_data(l)
end

function LastWrongDies:get_competitive_word(l)
    while true do
        local retry = false
        local data = {
            lobby_type = "10v10",
            lobby_uuid = l.id,
            category_in = l.tags["category"]
        }
        local headers = {
            "SERVICE_TOKEN", "b80f6ba2-e342-428a-b4f1-791bdebe2142",
            "SERVICE_SECRET", "tSCU2GWuqLWT0VtkpF2u1a6DilCdhGcm",
            "Content-Type", "application/json"
        }
        local status, body = system.http_request("POST", "https://quiz-service-tbrvq.ondigitalocean.app/api/v1/entries", {}, headers, system.encode_json(data))
        if status ~= 200 then
            print("Error: Got status " .. status .. " and body " .. body)
            retry = true
        end

        if not retry then
            local response = system.decode_json(body)
            local questions = response["questions"]
            if questions == nil then
                print("No more words available: questions is null")
                l.broadcast_message({ type = "no_more_words" })
                return l, nil, nil, { error = "No more words available." }
            end

            local question_data = questions
            local word = string.upper(tostring(question_data[1]["answer"]))
            local hint = string.upper(tostring(question_data[1]["question"]))
            local is_valid = true
            for i = 1, #word do
                local char = word:sub(i, i)
                if char ~= " " and not char:match("[A-Za-z]") then
                    is_valid = false
                    break
                end
            end
            if is_valid then
                l.public_data["guessed"] = ""
                for i = 1, #word do
                    local letter = word:sub(i, i)
                    l.public_data["guessed"] = l.public_data["guessed"] .. (letter == " " and " " or "_")
                end
                return l, word, hint
            else
                retry = true
            end
        end
    end
end

function LastWrongDies:guess_letter(l, peerID, letter)
    letter = string.upper(tostring(letter))
    if #letter ~= 1 then return { error = "Too many letters." } end
    if not self:is_letter(letter) then return { error = "Invalid letter." } end
    local err = turn.validate_game_state_is("playing")
    if err then return err end

    if l.public_data["pressed"][letter] then
        return { error = "Letter was already pressed." }
    end

    local pressed = l.public_data["pressed"]
    pressed[letter] = peerID
    l.public_data["pressed"] = pressed

    local player_guessed = l.public_data["players_guessed"]
    player_guessed[peerID] = true
    l.public_data["players_guessed"] = player_guessed

    local peer_name = string.upper(tostring(l.peers[peerID].user_data["name"]))
    local word = l.private_data["word"]
    if not string.find(word, letter, 1, true) then
        lobby.broadcast_chat(string.format("%s guessed the wrong letter, %s", peer_name, letter))
        self:take_damage(l, peerID)
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

    local total_points = (l.peers[peerID].public_data["total_points"] or 0) + points
    self:set_points(l, l.peers[peerID], total_points)
    lobby.broadcast_chat(string.format(
        "%s guessed letter %s. Gained %d points. Total: %d",
        peer_name, letter, points, total_points
    ))

    if l.public_data["guessed"] == word and l.public_data["game_state"] ~= "over" then
        lobby.broadcast_chat("Starting next round...")
        l.public_data["game_state"] = "new_round"
        lobby.start_timer("_on_timer_next_round", 1)
    end
    return
end

function LastWrongDies:show_letter_hint(l, peerID)
    return { error = "Letter hints are not supported in all_or_nothing mode." }
end

function LastWrongDies:show_hint(l, peerID)
    return { error = "Hints are not supported in all_or_nothing mode." }
end

function LastWrongDies:take_damage(l, peerID)
    l.public_data["health"] = l.public_data["health"] - 1
    if l.public_data["health"] <= 0 then
        if peerID == "" then
            local players_which_did_not_guess = {}
            for k, _ in pairs(l.peers) do
                if l.public_data["players_guessed"][k] == false then
                    table.insert(players_which_did_not_guess, k)
                end
            end
            local random_index = math.random(1, #players_which_did_not_guess)
            peerID = players_which_did_not_guess[random_index]
        end
        self:set_state(l, l.peers[peerID], "dead")
        self:check_game_end(l, peerID, "lost")
        local peer_name = string.upper(tostring(l.peers[peerID].user_data["name"]))
        lobby.broadcast_chat(string.format("%s is dead", peer_name))
        if l.public_data["game_state"] ~= "over" then
            lobby.broadcast_chat(string.format("recreating body"))
            l.public_data["game_state"] = "recreating_body"
            lobby.start_timer("_on_timer_recreate_body", 5)
        end
    end
    return
end

function LastWrongDies:check_game_end(l, peerID, newState)
    -- if only one player is alive, the game ends
    local alive_count = 0
    for k, _ in pairs(l.peers) do
        if l.peers[k].public_data["state"] == "alive" then
            alive_count = alive_count + 1
        end
    end
    if alive_count == 1 then
        for k, _ in pairs(l.peers) do
            if l.peers[k].public_data["state"] == "alive" then
                local winning_message = string.format("%s won!", l.peers[k].user_data["name"])
                lobby.broadcast_chat(winning_message)
                l.public_data["announcement_message"] = winning_message
                self:set_state(l, l.peers[k], "won")
                break
            end
        end
        l.public_data["word"] = l.private_data["word"]
        l.public_data["game_state"] = "over"
        lobby.start_timer("_on_timer_restart_game", 10)
    end
end

function LastWrongDies:on_tick(l, tickrate)
    local current_time = system.get_time_since_epoch()
    for _, peer in pairs(l.peers) do
        if current_time - peer.private_data["timer"] >= 10000 then
            print(string.format("Peer %s timed out. Taking damage.", peer.user_data["name"]))
            self:take_damage(l, peer.id)
        end
    end
end

function LastWrongDies:on_timer_next_round(l)
    local l_new, word, hint = self:get_competitive_word(l)
    l = l_new
    if l.error then return l end
    l.private_data["word"] = word
    l.public_data["hint"] = hint
    l.public_data["pressed"] = {}
    l.public_data["game_state"] = "playing"
    l.public_data["turn_timestamp"] = system.get_time_since_epoch()
    lobby.start_timer("_on_timer_guess_timeout", 10)
    return
end

function LastWrongDies:on_timer_restart_game(l)
    l.public_data["game_state"] = "setup"
    return
end

function LastWrongDies:on_timer_guess_timeout(l)
    l = lobby.get()
    if l.public_data["game_state"] ~= "playing" then
        return
    end
    l.public_data["turn_timestamp"] = system.get_time_since_epoch()
    local players_which_did_not_guess_count = 0
    for k, _ in pairs(l.peers) do
        if l.public_data["players_guessed"][k] == false and l.peers[k].public_data["state"] == "alive" then
            players_which_did_not_guess_count = players_which_did_not_guess_count + 1
        end
    end
    if players_which_did_not_guess_count > 0 then
        self:take_damage(l, "")
    end
    for k, _ in pairs(l.peers) do
        local players_guessed = l.public_data["players_guessed"]
        players_guessed[k] = false
        l.public_data["players_guessed"] = players_guessed
    end
    lobby.start_timer("_on_timer_guess_timeout", 10)
end

function LastWrongDies:on_timer_recreate_body(l)
    l.public_data["game_state"] = "playing"
    l.public_data["health"] = 6
    l.public_data["turn_timestamp"] = system.get_time_since_epoch()
    lobby.start_timer("_on_timer_guess_timeout", 10)
    return l
end

function LastWrongDies:set_initial_data(l)
    l.public_data["turn_timestamp"] = system.get_time_since_epoch()
    l.public_data["game_state"] = "playing"
    l.public_data["health"] = 6
    l.public_data["dealer"] = nil
    l.public_data["dealer_idx"] = nil
    l.public_data["pressed"] = {}
    l.private_data["tried_words"] = {}
    return l
end

function LastWrongDies:on_left(l, peerID)
    return
end

function LastWrongDies:on_join(l, peerID)
    if l.public_data["game_state"] == "setup" then
        return
    end
    local players_guessed = l.public_data["players_guessed"]
    players_guessed[peerID] = false
    l.public_data["players_guessed"] = players_guessed
    local saved_player_state = l.private_data["players_states"][l.peers[peerID].platform .. ":" .. l.peers[peerID].platform_id]
    if saved_player_state == nil then
        self:set_state(l, l.peers[peerID], "alive")
    else
        l.peers[peerID].public_data["state"] = saved_player_state
    end
    local saved_player_points = l.private_data["players_points"][l.peers[peerID].platform .. ":" .. l.peers[peerID].platform_id]
    if saved_player_points == nil then
        self:set_points(l, l.peers[peerID], 0)
    else
        l.peers[peerID].public_data["total_points"] = saved_player_points
    end
    return
end

function LastWrongDies:is_letter(letter)
    local b = letter:byte()
    return b >= string.byte('A') and b <= string.byte('Z')
end

return LastWrongDies