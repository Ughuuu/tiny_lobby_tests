local lobby = require("lobby")
local helper = require("helper")

local turn = {}

function turn.increment_dealer(l)
    if l.public_data["dealer_idx"] == nil then
        l.public_data["dealer_idx"] = -1
    end
    l.public_data["dealer_idx"] = l.public_data["dealer_idx"] + 1
    local peers_ordered = helper.peers_ordered(l)
    local dealer_idx = l.public_data["dealer_idx"] % helper.map_length(l.Peers)
    local dealer_key = peers_ordered[dealer_idx + 1] -- Lua is 1-indexed!
    l.public_data["dealer"] = l.Peers[dealer_key].ID
    return l
end

function turn.increment_turn(l)
    if l.public_data["turn_idx"] == nil then
        l.public_data["turn_idx"] = -1
    end
    l.public_data["turn_idx"] = l.public_data["turn_idx"] + 1
    local peers_ordered = helper.peers_ordered(l)
    local to_select_idx = (l.public_data["turn_idx"] + l.public_data["dealer_idx"]) % helper.map_length(l.Peers)
    local selected_peer_key = peers_ordered[to_select_idx + 1] -- Lua is 1-indexed!
    l.public_data["turn"] = l.Peers[selected_peer_key].ID
    return l
end

function turn.validate_game_state_is(state_to_validate)
    if lobby.get().public_data["game_state"] ~= state_to_validate then
        return { error = "Game state is not " .. state_to_validate }
    end
end

return turn
