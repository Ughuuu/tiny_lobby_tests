local lobby = require("lobby")
local turn = require("turn")

local api = {}

function api.start_game()
    local l = lobby.get()
    if l.peers[l.calling_peer_id].id ~= l.host then
        return { error = "You are not the host" }
    end
    if l.public_data["game_state"] ~= "setup" then
        return { error = "Game already started" }
    end
    lobby.start_timer("_on_timer_question_expired", 3)
    l = api.set_initial_data(l)
end

function api.set_initial_data(l)
    for k, _ in pairs(l.peers) do
        l.peers[k].public_data["points"] = 0
        l.peers[k].private_data["answer"] = -1
    end
    l.public_data["game_state"] = "playing"
    l.public_data["correct_answer"] = -1
    l = turn.increment_dealer(l)
    l = api.update_question(l)
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
end

function api.update_question(l)
    l = turn.increment_turn(l)
    print(l.public_data["turn_idx"])
    local questions = {
        "What is the capital of France?", 
        "What is the capital of Germany?", 
        "What is the capital of Spain?", 
        "What is the capital of Italy?", 
        "What is the capital of the United Kingdom?", 
        "What is the capital of the United States?", 
        "What is the capital of Canada?", 
        "What is the capital of Mexico?", 
        "What is the capital of Brazil?", 
        "What is the capital of Argentina?"
    }
    local answers = {
        {"Paris", "London", "Berlin", "Madrid"},
        {"London", "Paris", "Berlin", "Madrid"},
        {"Paris", "Madrid", "London", "Berlin"},
        {"Rome", "Paris", "London", "Berlin"},
        {"Berlin", "Paris", "London", "Madrid"},
        {"Washington D.C.", "Paris", "London", "Berlin"},
        {"Ottawa", "Paris", "London", "Berlin"},
        {"Mexico City", "Paris", "London", "Berlin"},
        {"Brasilia", "Paris", "London", "Berlin"},
        {"Buenos Aires", "Paris", "London", "Berlin"}
    }
    local correct_answers = {0, 2, 1, 0, 2, 0, 0, 0, 0, 0}
    
    l.public_data["question"] = questions[l.public_data["turn_idx"]]
    l.public_data["answers"] = answers[l.public_data["turn_idx"]]
    l.public_data["correct_answer"] = -1
    l.private_data["correct_answer"] = correct_answers[l.public_data["turn_idx"]]
    return l
end

function api.on_timer_restart_game()
    local l = lobby.get()
    l.public_data["game_state"] = "setup"
end

function api.on_timer_question_reset()
    local l = lobby.get()
    local is_over = false
    for k, _ in pairs(l.peers) do
        if l.public_data["turn_idx"] >= 9 then
            is_over = true
        end
        l.peers[k].private_data["answer"] = -1
    end
    if is_over then
        l.public_data["game_state"] = "over"
        lobby.save(l)
        return lobby.start_timer("_on_timer_restart_game", 2)
    end
    l.public_data["game_state"] = "playing"
    l = api.update_question(l)
    return lobby.start_timer("_on_timer_question_expired", 3)
end

function api.on_timer_question_expired()
    local l = lobby.get()
    for k, _ in pairs(l.peers) do
        if l.peers[k].private_data["answer"] == l.private_data["correct_answer"] then
            l.peers[k].public_data["points"] = l.peers[k].public_data["points"] + 1
        end
    end
    l.public_data["game_state"] = "quesiton_over"
    l.public_data["correct_answer"] = l.private_data["correct_answer"]
    return lobby.start_timer("_on_timer_question_reset", 1)
end

return api
