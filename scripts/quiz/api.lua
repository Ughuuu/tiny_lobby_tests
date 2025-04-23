local lobby = require("lobby")
local turn = require("turn")
local system = require("system")
local api = {}

function api.start_game()
    local l = lobby.get()
    if l.peers[l.calling_peer_id].id ~= l.host then
        return { error = "You are not the host" }
    end
    if l.public_data["game_state"] ~= "setup" then
        return { error = "Game already started" }
    end
    local data = {
        lobby_type = "10v10",
        lobby_uuid = l.id .. math.random(1, 1000),
        entry_count = 10,
        language_code = "en",
        question_type = "any",
        category_in = l.tags["category"]
    }
    local headers = {
        "SERVICE_TOKEN", "b80f6ba2-e342-428a-b4f1-791bdebe2142",
        "SERVICE_SECRET", "tSCU2GWuqLWT0VtkpF2u1a6DilCdhGcm",
        "Content-Type", "application/json"
    }
    print("Sending request to quiz service")
    local status, body = system.http_request("POST", "https://quiz-service-tbrvq.ondigitalocean.app/api/v1/entries", {}, headers, system.encode_json(data))
    if status ~= 200 then
        print("Error: Got status " .. status .. " and body " .. body)
        return { error = "Failed to fetch questions." }
    end
    local questions = system.decode_json(body)["questions"]
    if not questions or #questions == 0 then
        print("Error: No questions received")
        return { error = "No questions available." }
    end
    l.private_data["questions"] = questions
    l = api.set_initial_data(l)
    return
end

function api.set_initial_data(l)
    for k, _ in pairs(l.peers) do
        l.peers[k].public_data["total_points"] = 0
        l.peers[k].private_data["answer"] = -1
    end
    l.public_data["game_state"] = "playing"
    l.public_data["turn_timestamp"] = system.get_time_since_epoch()
    l.public_data["correct_answer"] = -1
    l = turn.increment_dealer(l, 1)
    l = api.prepare_questions(l)
    l = api.update_question(l)
    lobby.start_timer("_on_timer_question_expired", 7)
    return l
end

function api.prepare_questions(l)
    local questions = l.private_data["questions"] or {}
    local prepared_questions = {}
    local all_answers = {}
    -- Collect all valid answers, ensuring uniqueness
    local seen_answers = {}
    for _, q in ipairs(questions) do
        if q["answer"] and q["answer"] ~= "" and not seen_answers[q["answer"]] then
            table.insert(all_answers, q["answer"])
            seen_answers[q["answer"]] = true
        end
    end
    for i, q in ipairs(questions) do
        if q["question"] and q["answer"] and q["answer"] ~= "" then
            local question_text = q["question"]
            local correct_answer = q["answer"]
            -- Select available answers
            local incorrect_answers = {}
            local available_answers = {}
            for _, ans in ipairs(all_answers) do
                if ans ~= correct_answer then
                    table.insert(available_answers, ans)
                end
            end
            -- Shuffle available answers
            for j = 1, #available_answers do
                local k = math.random(j, #available_answers)
                available_answers[j], available_answers[k] = available_answers[k], available_answers[j]
            end
            -- Take up to 3 incorrect answers
            for j = 1, math.min(3, #available_answers) do
                table.insert(incorrect_answers, available_answers[j])
            end
            -- If fewer than 3 incorrect answers, cycle through available_answers again
            local available_idx = 1
            while #incorrect_answers < 3 do
                if #available_answers > 0 then
                    -- Reuse an answer from available_answers, cycling through
                    local next_answer = available_answers[(available_idx - 1) % #available_answers + 1]
                    table.insert(incorrect_answers, next_answer)
                    available_idx = available_idx + 1
                else
                    -- Edge case: no available answers (e.g., only one unique answer); reuse correct answer as a last resort
                    table.insert(incorrect_answers, correct_answer)
                end
            end
            -- Combine and shuffle answers
            local answers = {correct_answer, incorrect_answers[1], incorrect_answers[2], incorrect_answers[3]}
            local shuffled_indices = {0, 1, 2, 3}
            for j = 1, 4 do
                local k = math.random(j, 4)
                shuffled_indices[j], shuffled_indices[k] = shuffled_indices[k], shuffled_indices[j]
            end
            local shuffled_answers = {}
            local correct_answer_idx = -1
            for j, idx in ipairs(shuffled_indices) do
                shuffled_answers[j] = answers[idx + 1]
                if idx == 0 then
                    correct_answer_idx = j - 1
                end
            end
            -- Store prepared question
            table.insert(prepared_questions, {
                question = question_text,
                answers = shuffled_answers,
                correct_answer = correct_answer_idx
            })
        end
    end
    l.private_data["prepared_questions"] = prepared_questions
    return l
end

function api.guess_answer(answer_id)
    if type(answer_id) ~= "number" then
        return { error = "Answer Id must be an integer." }
    end
    if answer_id < 0 or answer_id > 3 then
        return { error = "Answer Id must be between 0 and 3." }
    end
    local l = lobby.get()
    if l.public_data["game_state"] ~= "playing" then
        return { error = "Game has not started." }
    end
    if l.peers[l.calling_peer_id].private_data["answer"] ~= -1 then
        return { error = "You have already selected an answer." }
    end
    l.peers[l.calling_peer_id].private_data["answer"] = answer_id
    return
end

function api.update_question(l)
    l = turn.increment_turn(l, 1)
    local prepared_questions = l.private_data["prepared_questions"] or {}
    local turn_idx = l.public_data["turn_idx"] + 1
    if turn_idx > #prepared_questions then
        l.public_data["game_state"] = "over"
        return l
    end
    local current_question = prepared_questions[turn_idx]
    l.public_data["question"] = current_question.question
    l.public_data["answers"] = current_question.answers
    l.public_data["correct_answer"] = -1
    l.private_data["correct_answer"] = current_question.correct_answer
    return l
end

function api.on_timer_restart_game()
    local l = lobby.get()
    l.public_data["game_state"] = "setup"
    return
end

function api.on_timer_question_reset()
    local l = lobby.get()
    for k, _ in pairs(l.peers) do
        l.peers[k].private_data["answer"] = -1
    end
    l.public_data["game_state"] = "playing"
    l.public_data["turn_timestamp"] = system.get_time_since_epoch()
    l = api.update_question(l)
    return lobby.start_timer("_on_timer_question_expired", 7)
end

function api.on_timer_question_expired()
    local l = lobby.get()
    for k, _ in pairs(l.peers) do
        if l.peers[k].private_data["answer"] == l.private_data["correct_answer"] then
            if l.peers[k].public_data["total_points"] == nil then
                l.peers[k].public_data["total_points"] = 0
            end
            l.peers[k].public_data["total_points"] = l.peers[k].public_data["total_points"] + 1
        end
    end
    if l.public_data["game_state"] == "over" then
        return lobby.start_timer("_on_timer_restart_game", 1)
    end
    l.public_data["game_state"] = "question_over"
    l.public_data["correct_answer"] = l.private_data["correct_answer"]
    return lobby.start_timer("_on_timer_question_reset", 3)
end

return api