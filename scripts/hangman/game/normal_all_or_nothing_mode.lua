local turn = require("turn")
local helper = require("helper")
local lobby = require("lobby")
local system = require("system")
local GameMode = require("game/game_mode")

local CompetitiveAllOrNothing = setmetatable({}, { __index = GameMode })
CompetitiveAllOrNothing.__index = CompetitiveAllOrNothing

function CompetitiveAllOrNothing.new()
    local self = setmetatable(GameMode.new(), CompetitiveAllOrNothing)
    return self
end

function CompetitiveAllOrNothing:start_game(l)
    if l.peers[l.calling_peer_id].id ~= l.host then
        return { error = "You are not the host" }
    end
    if l.public_data["game_state"] ~= "setup" then
        return { error = "Game already started" }
    end
    local l_new, word, hint = self:get_competitive_word(l)
    l = l_new
    for k, _ in pairs(l.peers) do
        l.peers[k].public_data["total_points"] = 0
    end
    l.private_data["word"] = word
    l.public_data["hint"] = hint
    l.public_data["announcement_message"] = ""
    lobby.start_timer("_on_timer_guess_timeout", 10)
    return self:set_initial_data(l)
end

function CompetitiveAllOrNothing:get_competitive_word(l)
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
                for i, letter in ipairs(word) do
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

function CompetitiveAllOrNothing:guess_letter(l, peerID, letter)
    if not helper.is_letter(letter, l.tags["lang"]) then return { error = "Invalid letter." } end
    local err = turn.validate_game_state_is("playing")
    if err then return err end

    if l.public_data["pressed"][letter] then
        return { error = "Letter was already pressed." }
    end

    local pressed = l.public_data["pressed"]
    pressed[letter] = peerID
    l.public_data["pressed"] = pressed

    local peer_name = string.upper(tostring(l.peers[peerID].user_data["name"]))
    local word = l.private_data["word"]
    if not helper.array_contains(word, letter) then
        lobby.broadcast_chat(string.format("%s guessed the wrong letter, %s", peer_name, letter))
        self:take_damage(l, "")
        l.public_data["turn_timestamp"] = system.get_time_since_epoch()
        lobby.start_timer("_on_timer_guess_timeout", 10)
    return { error = "Letter is not in the word." }
    end

    local points = 0
    local guessed = l.public_data["guessed"]
    for i, char in ipairs(word) do
        if char == letter then
            points = points + 1
            guessed[i] = letter
        end
    end
    l.public_data["guessed"] = guessed

    local total_points = (l.peers[peerID].public_data["total_points"] or 0) + points
    l.peers[peerID].public_data["total_points"] = total_points
    lobby.broadcast_chat(string.format(
        "%s guessed letter %s. Gained %d points. Total: %d",
        peer_name, letter, points, total_points
    ))

    if helper.arrays_equal(l.public_data["guessed"], word) then
        lobby.broadcast_chat("Starting next round...")
        l.public_data["game_state"] = "new_round"
        lobby.start_timer("_on_timer_next_round", 1)
    else
        l.public_data["turn_timestamp"] = system.get_time_since_epoch()
        lobby.start_timer("_on_timer_guess_timeout", 10)
    end
    return
end

function CompetitiveAllOrNothing:show_letter_hint(l, peerID)
    return { error = "Letter hints are not supported in all_or_nothing mode." }
end

function CompetitiveAllOrNothing:show_hint(l, peerID)
    return { error = "Hints are not supported in all_or_nothing mode." }
end

function CompetitiveAllOrNothing:take_damage(l, peerID)
    if l.public_data["health"] <= 0 then
        return
    end
    l.public_data["health"] = l.public_data["health"] - 1
    if l.public_data["health"] <= 0 then
        l.public_data["game_state"] = "over"
        l.public_data["word"] = l.private_data["word"]
        local winning_message = "Players lost! Game over."
        local players = {}
        for k, _ in pairs(l.peers) do
            table.insert(players, {
                name = l.peers[k].user_data["name"],
                total_points = l.peers[k].public_data["total_points"] or 0
            })
        end
        table.sort(players, function(a, b)
            return a.total_points > b.total_points
        end)
        for _, player in ipairs(players) do
            winning_message = winning_message .. string.format("\n%s contributed %d letters", player.name, player.total_points)
        end
        l.public_data["announcement_message"] = winning_message
        lobby.start_timer("_on_timer_restart_game", 10)
    end
    return
end

function CompetitiveAllOrNothing:check_game_end(l, peerID, newState)
    -- AllOrNothing mode doesn't use per-player win/loss states
end

function CompetitiveAllOrNothing:on_timer_next_round(l)
    local l_new, word, hint = self:get_competitive_word(l)
    l = l_new
    l.public_data["turn_timestamp"] = system.get_time_since_epoch()
    if l.error then return l end
    l.private_data["word"] = word
    l.public_data["hint"] = hint
    l.public_data["pressed"] = {}
    l.public_data["game_state"] = "playing"
    return
end

function CompetitiveAllOrNothing:on_timer_restart_game(l)
    l.public_data["game_state"] = "setup"
    return
end

function CompetitiveAllOrNothing:on_timer_guess_timeout(l)
    l.public_data["turn_timestamp"] = system.get_time_since_epoch()
    self:take_damage(l, "")
    lobby.start_timer("_on_timer_guess_timeout", 10)
end

function CompetitiveAllOrNothing:on_tick(l, tickrate)
    local current_time = system.get_time_since_epoch()
    for _, peer in pairs(l.peers) do
        if current_time - peer.private_data["timer"] >= 10000 then
            print(string.format("Peer %s timed out. Taking damage.", peer.user_data["name"]))
            self:take_damage(l, peer.id)
        end
    end
end

function CompetitiveAllOrNothing:set_initial_data(l)
    l.public_data["turn_timestamp"] = system.get_time_since_epoch()
    l.public_data["game_state"] = "playing"
    l.public_data["health"] = 6
    l.public_data["dealer"] = nil
    l.public_data["dealer_idx"] = nil
    l.public_data["pressed"] = {}
    l.private_data["tried_words"] = {}
    return l
end

function CompetitiveAllOrNothing:on_left(l, peerID)
    -- No-op
    return
end

return CompetitiveAllOrNothing
