local turn = require("turn")
local helper = require("helper")
local lobby = require("lobby")
local system = require("system")
local GameMode = require("game/game_mode")

local CompetitiveAbunchHanging = setmetatable({}, { __index = GameMode })
CompetitiveAbunchHanging.__index = CompetitiveAbunchHanging

function CompetitiveAbunchHanging.new()
    local self = setmetatable(GameMode.new(), CompetitiveAbunchHanging)
    return self
end

function CompetitiveAbunchHanging:set_points(l, peer, points)
    local player_points = l.private_data["players_points"]
    player_points[peer.platform .. ":" .. peer.platform_id] = points
    l.private_data["players_points"] = player_points
    peer.public_data["total_points"] = points

    local highest_points_player = l.public_data["highest_points_player"]
    if points > (highest_points_player["points"] or 0) then
        highest_points_player["name"] = peer.user_data["name"]
        highest_points_player["points"] = points
        l.public_data["highest_points_player"] = highest_points_player
    end
end

function CompetitiveAbunchHanging:set_state(l, peer, state)
    local player_states = l.private_data["players_states"]
    player_states[peer.platform .. ":" .. peer.platform_id] = state
    l.private_data["players_states"] = player_states
    peer.public_data["state"] = state
    if state == "lost" then
        l.broadcast_chat(string.format("%s lost the game and was kicked!", peer.user_data["name"]))
        l.start_timer("_on_timer_kick_peer", 8, peer.id)
    end
end

function CompetitiveAbunchHanging:start_game(l)
    if l.public_data["game_state"] ~= "setup" then
        return { error = "Game already started" }
    end
    l.private_data["players_states"] = {}
    l.private_data["players_points"] = {}
    l.private_data["current_word_list"] = {}
    l.private_data["categories"] = self:get_categories()
    l.public_data["highest_points_player"] = {}
    for k, _ in pairs(l.peers) do
        self:set_points(l, l.peers[k], 0)
        self:set_state(l, l.peers[k], "")
        l.peers[k].public_data["health"] = 6
        l.peers[k].private_data["word_list"] = {}
        l = self:assign_random_word(l, k)
    end
    l.private_data["words"] = {}
    l.public_data["turn_timestamp"] = system.get_time_since_epoch()
    l.public_data["game_state"] = "playing"
    return l
end

function CompetitiveAbunchHanging:get_categories()
    local headers = {
        "SERVICE_TOKEN", "b80f6ba2-e342-428a-b4f1-791bdebe2142",
        "SERVICE_SECRET", "tSCU2GWuqLWT0VtkpF2u1a6DilCdhGcm",
        "Content-Type", "application/json"
    }
    local status, body = system.http_request("GET", "https://quiz-service-tbrvq.ondigitalocean.app/api/v1/metadata", {}, headers)
    if status == 200 then
        local response = system.decode_json(body)
        return response["categories"] or {"Geography"}
    else
        print("Error fetching metadata: Status " .. status .. " Body: " .. body)
        return {"Geography"}
    end
end

function CompetitiveAbunchHanging:get_competitive_words(l, peerID, count)
    local retry = true
    local word_list = l.peers[peerID].private_data["word_list"] or {}
    local current_category = l.tags["category"] or "Geography"
    while retry do
        local data = {
            lobby_type = "10v10",
            lobby_uuid = l.id,
            category_in = current_category
        }
        local headers = {
            "SERVICE_TOKEN", "b80f6ba2-e342-428a-b4f1-791bdebe2142",
            "SERVICE_SECRET", "tSCU2GWuqLWT0VtkpF2u1a6DilCdhGcm",
            "entry_count", tostring(count),
            "Content-Type", "application/json"
        }
        local status, body = system.http_request("POST", "https://quiz-service-tbrvq.ondigitalocean.app/api/v1/entries", {}, headers, system.encode_json(data))
        if status ~= 200 then
            print("Error: Got status " .. status .. " and body " .. body)
        else
            local response = system.decode_json(body)
            local questions = response["questions"]
            if questions == nil then
                print("No more words available in category: " .. current_category)
                local categories = l.private_data["categories"] or {"Geography"}
                local available_categories = {}
                for _, cat in ipairs(categories) do
                    if cat ~= current_category then
                        table.insert(available_categories, cat)
                    end
                end
                l.private_data["categories"] = available_categories
                if #available_categories == 0 then
                    l.broadcast_message({ type = "no_more_words" })
                    return l, nil, { error = "No more words available in any category." }
                end
                current_category = available_categories[math.random(1, #available_categories)]
                l.tags["category"] = current_category
                print("Switching to new category: " .. current_category)
            else
                for _, question_data in ipairs(questions) do
                    local word = helper.string_to_array(string.upper(question_data["answer"]))
                    local hint = string.upper(tostring(question_data["question"]))
                    local is_valid = true
                    for _, char in ipairs(word) do
                        if char ~= " " and not helper.is_letter(char, l.tags["lang"]) then
                            is_valid = false
                            break
                        end
                    end
                    if is_valid then
                        table.insert(word_list, { word = word, hint = hint })
                    end
                end
                if #word_list >= count then
                    retry = false
                end
            end
        end
    end
    -- Update all players' word lists
    for k, _ in pairs(l.peers) do
        if k ~= peerID then
            local peer_word_list = l.peers[k].private_data["word_list"] or {}
            for _, word_entry in ipairs(word_list) do
                table.insert(peer_word_list, word_entry)
            end
            l.peers[k].private_data["word_list"] = peer_word_list
        end
    end
    l.peers[peerID].private_data["word_list"] = word_list
    return l, word_list
end

function CompetitiveAbunchHanging:assign_random_word(l, peerID)
    local word_list = l.peers[peerID].private_data["word_list"] or {}
    if not word_list or #word_list == 0 then
        local l_new, new_word_list = self:get_competitive_words(l, peerID, 2)
        l = l_new
        if l.error then return l end
        word_list = new_word_list
        l.private_data["current_word_list"] = word_list
    end
    local index = math.random(1, #word_list)
    local selected = word_list[index]
    table.remove(word_list, index)
    -- print(string.format("Selected word: %s (Hint: %s)", selected.word, selected.hint))
    l.peers[peerID].private_data["word_list"] = word_list
    return self:set_competitive_word(l, peerID, selected.word, selected.hint)
end

function CompetitiveAbunchHanging:guess_letter(l, peerID, letter)
    if not helper.is_letter(letter, l.tags["lang"]) then return { error = "Invalid letter." } end
    local err = turn.validate_game_state_is("playing")
    if err then return err end

    if l.peers[peerID].private_data["pressed"][letter] then
        return { error = "Letter was already pressed." }
    end
    local pressed = l.peers[peerID].private_data["pressed"]
    pressed[letter] = true
    l.peers[peerID].private_data["pressed"] = pressed

    local word = l.peers[peerID].private_data["word"]
    if not helper.array_contains(word, letter) then
        self:take_damage(l, peerID)
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

    if helper.arrays_equal(guessed, word) then
        l.peers[peerID].private_data["timer"] = system.get_time_since_epoch()
        self:set_state(l, l.peers[peerID], "waiting")
        l.start_timer("_on_timer_next_word", 2, peerID)
    end
    l.peers[peerID].private_data["timer"] = system.get_time_since_epoch()
    return l
end

function CompetitiveAbunchHanging:show_letter_hint(l, peerID)
    local hints_used = l.peers[peerID].private_data["letter_hints_used"] or 0
    if hints_used >= 2 then
        return { error = "You have already used the maximum number of letter hints." }
    end
    l.peers[peerID].private_data["letter_hints_used"] = hints_used + 1
    local word = l.peers[peerID].private_data["word"]
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
            l.peers[peerID].private_data["timer"] = system.get_time_since_epoch()
            self:set_state(l, l.peers[peerID], "waiting")
            l.start_timer("_on_timer_next_word", 2, peerID)
        end
        return next_letter
    end
    return nil
end

function CompetitiveAbunchHanging:show_hint(l, peerID)
    local penalty = math.floor(#l.peers[peerID].private_data["word"] / 2)
    local total_points = (l.peers[peerID].public_data["total_points"] or 0) - penalty
    self:set_points(l, l.peers[peerID], total_points)
    l.peers[peerID].private_data["timer"] = system.get_time_since_epoch()
    l.peers[peerID].private_data["hint_used"] = true
    return l.peers[peerID].private_data["hint"]
end

function CompetitiveAbunchHanging:take_damage(l, peerID)
    if l.peers[peerID].public_data["health"] <= 0 then
        return
    end
    l.peers[peerID].public_data["health"] = l.peers[peerID].public_data["health"] - 1
    if l.peers[peerID].public_data["health"] <= 0 then
        self:set_state(l, l.peers[peerID], "lost")
    end
    l.peers[peerID].private_data["timer"] = system.get_time_since_epoch()
    return
end

function CompetitiveAbunchHanging:on_timer_kick_peer(l, peerID)
    l.kick_peer(peerID)
    return l
end

function CompetitiveAbunchHanging:set_competitive_word(l, peerID, word, hint)
    if l.peers[peerID].public_data["state"] ~= "lost" then
        self:set_state(l, l.peers[peerID], "")
        l.peers[peerID].private_data["timer"] = system.get_time_since_epoch()
    end
    local guessed = {}
    l.peers[peerID].private_data["word"] = word
    l.peers[peerID].private_data["hint"] = hint
    l.peers[peerID].private_data["pressed"] = {}
    l.peers[peerID].private_data["hint_used"] = false
    l.peers[peerID].private_data["letter_hints_used"] = 0

    for _, letter in ipairs(word) do
        if letter == " " then
            table.insert(guessed, ' ')
        else
            table.insert(guessed, '_')
        end
    end
    l.peers[peerID].private_data["guessed"] = {}
    return l
end

function CompetitiveAbunchHanging:on_join(l, peerID)
    if l.public_data["game_state"] == "setup" then
        return
    end
    local err = turn.validate_game_state_is("playing")
    if err then return err end
    self:set_state(l, l.peers[peerID], "")
    self:set_points(l, l.peers[peerID], 0)
    l.peers[peerID].public_data["health"] = 6
    l.peers[peerID].private_data["pressed"] = {}
    l.peers[peerID].private_data["hint_positions"] = {}
    l.peers[peerID].private_data["word_list"] = l.private_data["current_word_list"] or {}
    l = self:assign_random_word(l, peerID)
    l.public_data["turn_timestamp"] = system.get_time_since_epoch()
    l.peers[peerID].private_data["timer"] = system.get_time_since_epoch()
    return l
end

function CompetitiveAbunchHanging:on_timer_next_word(l, peerID)
    self:set_state(l, l.peers[peerID], "playing")
    l = self:assign_random_word(l, peerID)
    l.peers[peerID].private_data["pressed"] = {}
    l.peers[peerID].private_data["hint_positions"] = {}
    return
end

function CompetitiveAbunchHanging:on_left(l, peerID)
    -- No-op
    return
end

function CompetitiveAbunchHanging:on_tick(l, tickrate)
    local current_time = system.get_time_since_epoch()
    for _, peer in pairs(l.peers) do
        if current_time - peer.private_data["timer"] >= 10000 then
            self:take_damage(l, peer.id)
        end
    end
end

return CompetitiveAbunchHanging
