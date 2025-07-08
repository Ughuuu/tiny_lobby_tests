local turn = require("turn")
local helper = require("helper")
local lobby = require("lobby")
local system = require("system")
local GameMode = require("game/game_mode")

local NormalAbunchHanging = setmetatable({}, { __index = GameMode })
NormalAbunchHanging.__index = NormalAbunchHanging

function NormalAbunchHanging.new()
    local self = setmetatable(GameMode.new(), NormalAbunchHanging)
    return self
end

function NormalAbunchHanging:set_points(l, peer, points)
    local player_points = l.private_data["players_points"]
    player_points[peer.platform .. ":" .. peer.platform_id] = points
    l.private_data["players_points"] = player_points
    peer.public_data["total_points"] = points
end

function NormalAbunchHanging:set_state(l, peer, state)
    local player_states = l.private_data["players_states"]
    player_states[peer.platform .. ":" .. peer.platform_id] = state
    l.private_data["players_states"] = player_states
    peer.public_data["state"] = state
    if state == "won" then
        l.broadcast_chat(string.format("%s guessed the word!", peer.user_data["name"]))
    elseif state == "lost" then
        l.broadcast_chat(string.format("%s lost the game!", peer.user_data["name"]))
    end
end

function NormalAbunchHanging:start_game(l)
    if l.peers[l.calling_peer_id].id ~= l.host then
        return { error = "You are not the host" }
    end
    if l.public_data["game_state"] ~= "setup" then
        return { error = "Game already started" }
    end
    local l_new, word, hint = self:get_competitive_word(l)
    l = l_new
    if l.error then return l end
    l.private_data["word"] = word
    l.public_data["hint"] = hint
    l.private_data["players_states"] = {}
    l.private_data["players_points"] = {}
    for k, _ in pairs(l.peers) do
        self:set_points(l, l.peers[k], 0)
        self:set_state(l, l.peers[k], "")
        l.peers[k].public_data["health"] = 6
        l = self:set_competitive_word(l, k, word, hint)
    end
    l.private_data["words"] = {}
    l.public_data["announcement_message"] = ""
    return self:set_initial_data(l)
end

function NormalAbunchHanging:guess_letter(l, peerID, letter)
    if not helper.is_letter(letter, l.tags["lang"]) then return { error = "Invalid letter." } end
    local err = turn.validate_game_state_is("playing")
    if err then return err end

    if l.peers[peerID].private_data["pressed"][letter] then
        return { error = "Letter was already pressed." }
    end
    local pressed = l.peers[peerID].private_data["pressed"]
    pressed[letter] = true
    l.peers[peerID].private_data["pressed"] = pressed

    local peer_name = tostring(l.peers[peerID].user_data["name"])
    local word = l.private_data["word"]
    if not helper.array_contains(word, letter) then
        l.peers[peerID].public_data["health"] = l.peers[peerID].public_data["health"] - 1
        if l.peers[peerID].public_data["health"] <= 0 then
            self:set_state(l, l.peers[peerID], "lost")
            self:check_game_end(l)
        end
        l.broadcast_chat(string.format("%s guessed the wrong letter", peer_name))
    end

    local points = 0
    local guessed = l.peers[peerID].private_data["guessed"]
    for i, char in ipairs(word) do
        if char == letter then
            points = points + 1
            guessed[i] = letter
        end
    end
    l.peers[peerID].private_data["guessed"] = guessed
    local total_points = (l.peers[peerID].public_data["total_points"] or 0) + points
    self:set_points(l, l.peers[peerID], total_points)

    l.broadcast_chat(string.format(
        "%s guessed a letter. Gained %d points. Total: %d",
        peer_name, points, total_points
    ))
    if self:check_single_winner(l) then
        l.public_data["game_state"] = "over"
        l.start_timer("_on_timer_restart_game", 10)
        return
    end
    if helper.arrays_equal(guessed, word) then
        self:set_state(l, l.peers[peerID], "won")
        self:check_game_end(l)
    end
    l.peers[peerID].private_data["timer"] = system.get_time_since_epoch()
    return l
end

function NormalAbunchHanging:show_letter_hint(l, peerID)    
    local hints_used = l.peers[peerID].private_data["letter_hints_used"] or 0
    if hints_used >= 2 then
        return { error = "You have already used the maximum number of letter hints." }
    end
    l.peers[peerID].private_data["letter_hints_used"] = hints_used + 1
    local word = l.private_data["word"]
    local guessed = l.peers[peerID].private_data["guessed"] or {}

    l.peers[peerID].private_data["hint_positions"] = l.peers[peerID].private_data["hint_positions"] or {}

    local next_letter = nil
    for i, letter in ipairs(word) do
        if guessed[i] == "_" and not l.peers[peerID].private_data["hint_positions"][i] then
            next_letter = letter
            break
        end
    end

    if next_letter then
        local new_guessed = {}
        for i, char in ipairs(word) do
            if char == next_letter then
                table.insert(new_guessed, next_letter)
                l.peers[peerID].private_data["hint_positions"][i] = true
            else
                table.insert(new_guessed, guessed[i])
            end
        end
        l.peers[peerID].private_data["guessed"] = new_guessed
        local pressed = l.peers[peerID].private_data["pressed"]
        pressed[next_letter] = true
        l.peers[peerID].private_data["pressed"] = pressed
        local penalty = 1
        local total_points = (l.peers[peerID].public_data["total_points"] or 0) - penalty
        self:set_points(l, l.peers[peerID], total_points)
        if helper.arrays_equal(word, new_guessed) then
            self:set_state(l, l.peers[peerID], "won")
            self:check_game_end(l)
        end
        return next_letter
    end
    return nil
end

function NormalAbunchHanging:show_hint(l, peerID)
    local penalty = math.floor(#l.peers[peerID].private_data["word"] / 2)
    local total_points = (l.peers[peerID].public_data["total_points"] or 0) - penalty
    self:set_points(l, l.peers[peerID], total_points)
    l.peers[peerID].private_data["timer"] = system.get_time_since_epoch()
    return l.peers[peerID].private_data["hint"]
end

function NormalAbunchHanging:take_damage(l, peerID)
    if l.peers[peerID].public_data["health"] <= 0 then
        return
    end
    l.peers[peerID].public_data["health"] = l.peers[peerID].public_data["health"] - 1
    if l.peers[peerID].public_data["health"] <= 0 then
        self:set_state(l, l.peers[peerID], "lost")
        self:check_game_end(l)
    end
    l.peers[peerID].private_data["timer"] = system.get_time_since_epoch()
    return
end

function NormalAbunchHanging:check_game_end(l)
    local all_lost = true
    for k, _ in pairs(l.peers) do
        if l.peers[k].public_data["state"] == "won" or l.peers[k].public_data["state"] == "" then
            all_lost = false
            break
        end
    end
    if all_lost then
        local highest_points = 0
        local winners = {}
        for k, peer in pairs(l.peers) do
            local points = peer.public_data["total_points"] or 0
            local name = peer.user_data.name or ""
            if points > highest_points then
                highest_points = points
                winners = {name}
            elseif points == highest_points then
                table.insert(winners, name)
            end
        end
        local winning_message = ""
        if #winners > 1 then
            winning_message = string.format("The winners are: %s", table.concat(winners, ", "))
            l.broadcast_chat(winning_message)
        else
            winning_message = string.format("%s is the winner!", winners[1])
            l.broadcast_chat(winning_message)
        end
        l.public_data["announcement_message"] = winning_message
        l.public_data["game_state"] = "over"
        l.start_timer("_on_timer_restart_game", 5)
        return
    end
    if self:check_single_winner(l) then
        l.public_data["game_state"] = "over"
        l.start_timer("_on_timer_restart_game", 5)
        return
    end
    local all_finished = true
    for k, _ in pairs(l.peers) do
        if l.peers[k].public_data["state"] ~= "won" and l.peers[k].public_data["state"] ~= "lost" then
            all_finished = false
            break
        end
    end
    if all_finished then
        l.broadcast_chat("Starting next round...")
        l.start_timer("_on_timer_next_round", 1)
    end
end

function NormalAbunchHanging:on_timer_next_round(l)
    local l_new, word, hint = self:get_competitive_word(l)
    l = l_new
    if l.error then return l end
    l.private_data["word"] = word
    l.public_data["hint"] = hint
    for k, _ in pairs(l.peers) do
        l = self:set_competitive_word(l, k, word, hint)
    end
    l.public_data["game_state"] = "playing"
    return
end

function NormalAbunchHanging:on_timer_restart_game(l)
    for k, _ in pairs(l.peers) do
        l.peers[k].private_data["word"] = nil
        l.peers[k].public_data["health"] = 6
        l.peers[k].private_data["guessed"] = "_"
        l.peers[k].private_data["pressed"] = {}
    end
    l.public_data["game_state"] = "setup"
    return
    -- return self:set_initial_data(l)
end

function NormalAbunchHanging:on_timer_guess_timeout(l)
    -- Placeholder for guess timeout logic
end

function NormalAbunchHanging:set_initial_data(l)
    l.public_data["turn_timestamp"] = system.get_time_since_epoch()
    l.public_data["game_state"] = "playing"
    l.public_data["dealer"] = nil
    l.public_data["dealer_idx"] = nil
    l.public_data["turn"] = nil
    l.public_data["turn_idx"] = nil
    l.public_data["health"] = nil
    l.private_data["tried_words"] = {}
    return l
end

function NormalAbunchHanging:get_competitive_word(l)
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
            local word = helper.string_to_array(string.upper(question_data[1]["answer"]))
            local hint = tostring(question_data[1]["question"])
            local is_valid = true
            for _, char in ipairs(word) do
                if char ~= " " and not helper.is_letter(char, l.tags["lang"]) then
                    is_valid = false
                    break
                end
            end
            if is_valid then
                local guessed = {}
                for _, letter in ipairs(word) do
                    if letter == " " then
                        table.insert(guessed, " ")
                    else
                        table.insert(guessed, "_")
                    end
                end
                l.public_data["guessed"] = guessed
                return l, word, hint
            else
                retry = true
            end
        end
    end
end

function NormalAbunchHanging:set_competitive_word(l, peerID, word, hint)
    if l.peers[peerID].public_data["state"] ~= "lost" then
        self:set_state(l, l.peers[peerID], "")
        l.peers[peerID].private_data["timer"] = system.get_time_since_epoch()
    end
    l.peers[peerID].private_data["word"] = word
    l.peers[peerID].private_data["hint"] = hint
    l.peers[peerID].private_data["pressed"] = {}
    l.peers[peerID].private_data["letter_hints_used"] = 0
    local guessed = {}
    for _, letter in ipairs(word) do
        if letter == " " then
            table.insert(guessed, ' ')
        else
            table.insert(guessed, '_')
        end
    end
    l.peers[peerID].private_data["guessed"] = guessed
    return l
end

function NormalAbunchHanging:check_single_winner(l)
    local remaining_player = nil
    for k, _ in pairs(l.peers) do
        if l.peers[k].public_data["state"] ~= "lost" then
            if remaining_player ~= nil then
                return false
            end
            remaining_player = k
        end
    end
    local winner_points = l.peers[remaining_player].public_data["total_points"] or 0
    for k, _ in pairs(l.peers) do
        if k ~= remaining_player and l.peers[k].public_data["total_points"] >= winner_points then
            return false
        end
    end
    local winner_name = l.peers[remaining_player].user_data["name"] or ""
    local winning_message = string.format("%s is the winner!", winner_name)
    l.public_data["announcement_message"] = winning_message
    l.broadcast_chat(string.format("%s is the winner!", winner_name))
    return true
end

function NormalAbunchHanging:on_join(l, peerID)
    if l.public_data["game_state"] == "setup" then
        return
    end
    local err = turn.validate_game_state_is("playing")
    if err then return err end
    local saved_player_state = l.private_data["players_states"][l.peers[peerID].platform .. ":" .. l.peers[peerID].platform_id]
    if saved_player_state ~= nil then
        self:set_state(l, l.peers[peerID], "lost")
        self:set_points(l, l.peers[peerID], -1)
        return
    end
    l.peers[peerID].private_data["word"] = l.private_data["word"]
    l.peers[peerID].private_data["hint"] = l.public_data["hint"]
    l.peers[peerID].private_data["pressed"] = {}
    local guessed = {}
    local word = l.private_data["word"]
    for _, letter in ipairs(word) do
        if letter == " " then
            table.insert(guessed, ' ')
        else
            table.insert(guessed, '_')
        end
    end
    l.peers[peerID].private_data["guessed"] = guessed
    l.peers[peerID].public_data["health"] = 6
    self:set_state(l, l.peers[peerID], "")
    self:set_points(l, l.peers[peerID], 0)
    l.peers[peerID].private_data["timer"] = system.get_time_since_epoch()
    return
end

function NormalAbunchHanging:on_left(l, peerID)
    self:set_state(l, l.peers[peerID], "lost")
    self:check_game_end(l)
end

function NormalAbunchHanging:on_tick(l, tickrate)
    local current_time = system.get_time_since_epoch()
    for _, peer in pairs(l.peers) do
        if current_time - peer.private_data["timer"] >= 10000 then
            self:take_damage(l, peer.id)
        end
    end
end

return NormalAbunchHanging
