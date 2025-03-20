local helper = require("helper")
local turn = require("turn")
local lobby = require("lobby")

local api = {}

function api.start_game()
    local l = lobby.get()
    local ord = helper.peers_ordered(l)
    if l.peers[l.calling_peer_id].id ~= l.host then
        return { error = "You are not the host" }
    end
    if l.public_data["game_state"] ~= "setup" then
        return { error = "Game already started" }
    end
    for k, _ in pairs(l.peers) do
        l.peers[k].public_data["points"] = 0
        l.peers[k].public_data["total_points"] = 0
    end
    l = api.set_initial_data(l)
end

function api.place_piece(placementTileX, placementTileY)
    if type(placementTileX) ~= "number" or type(placementTileY) ~= "number" then
        return { error = "Placement Tile X and Y index must be a number." }
    end
    placementTileX = placementTileX + 1
    placementTileY = placementTileY + 1
    if placementTileX < 1 or placementTileX > 3 or placementTileY < 1 or placementTileY > 3 then
        return { error = "Placement Tile X and Y index must be a from 0 to 3." }
    end
    local l = lobby.get()
    if l.public_data["game_state"] ~= "playing" then
        return { error = "Game has not started." }
    end
    if l.public_data["turn"] ~= l.calling_peer_id then
        return { error = "Not your turn." }
    end
    local board = l.public_data["board"]
    -- Place piece on board
    if board[placementTileY][placementTileX] ~= 0 then
        return { error = "Piece already placed." }
    end
    board[placementTileY][placementTileX] = l.calling_peer_id
    l.public_data["board"] = board

    local line0 = board[1][1] == board[1][2] and board[1][2] == board[1][3] and board[1][1] ~= 0
    local line1 = board[2][1] == board[2][2] and board[2][2] == board[2][3] and board[2][1] ~= 0
    local line2 = board[3][1] == board[3][2] and board[3][2] == board[3][3] and board[3][1] ~= 0
    local column0 = board[1][1] == board[2][1] and board[2][1] == board[3][1] and board[1][1] ~= 0
    local column1 = board[1][2] == board[2][2] and board[2][2] == board[3][2] and board[1][2] ~= 0
    local column2 = board[1][3] == board[2][3] and board[2][3] == board[3][3] and board[1][3] ~= 0
    local diagonal0 = board[1][1] == board[2][2] and board[2][2] == board[3][3] and board[1][1] ~= 0
    local diagonal1 = board[1][3] == board[2][2] and board[2][2] == board[3][1] and board[1][3] ~= 0

    -- Check if peer won
    if line0 or line1 or line2 or column0 or column1 or column2 or diagonal0 or diagonal1 then
        return api.end_game(l, l.calling_peer_id, "won")
    end

    -- Check draw
    local is_draw = true
    for i = 1, 3 do
        for j = 1, 3 do
            if board[i][j] == 0 then
                is_draw = false
            end
        end
    end

    if is_draw then
        return api.end_game(l, l.calling_peer_id, "draw")
    end

    l = turn.increment_turn(l)
end

function api.set_initial_data(l)
    l.public_data["game_state"] = "playing"
    l.public_data["board"] = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
    }
    l = turn.increment_dealer(l)
    l.public_data["turn_idx"] = -1
    l = turn.increment_turn(l)
    return l
end

function api.end_game(l, state)
    l.public_data["game_state"] = state
    if state == "won" then
        l.peers[l.calling_peer_id].public_data["points"] = l.peers[l.calling_peer_id].public_data["points"] + 1
    end
    return start_timer("_on_timer_restart_game", 1)
end

function api.on_timer_restart_game()
    local l = lobby.get()
    if l.peers[l.calling_peer_id].public_data["points"] >= l.tags["max_points"] and l.tags["max_points"] ~= 0 then
        l.public_data["game_state"] = "setup"
    else
        l = api.set_initial_data(l)
    end
end

return api
