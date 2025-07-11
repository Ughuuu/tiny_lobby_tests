local lobby = require("lobby")

local turn = {}

function turn.increment_dealer(l, increment)
    if l.public_data["dealer_idx"] == nil then
        l.public_data["dealer_idx"] = -1
    end
    l.public_data["dealer_idx"] = l.public_data["dealer_idx"] + increment
    local dealer_idx = l.public_data["dealer_idx"] % l.peers_count
    l.public_data["dealer"] = l.peers_ordered[dealer_idx + 1].id
    return l
end

function turn.increment_turn(l, increment)
    if l.public_data["turn_idx"] == nil then
        l.public_data["turn_idx"] = -1
    end
    l.public_data["turn_idx"] = l.public_data["turn_idx"] + increment
    local to_select_idx = (l.public_data["turn_idx"] + l.public_data["dealer_idx"]) % l.peers_count
    l.public_data["turn"] = l.peers_ordered[to_select_idx + 1].id
    return l
end

function turn.validate_game_state_is(state_to_validate)
    local l = lobby.get()
    if l.public_data["game_state"] ~= state_to_validate then
        return { error = "Game state is not " .. state_to_validate }
    end
    return
end

return turn
