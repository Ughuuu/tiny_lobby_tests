local turn = require("turn")
local lobby = require("lobby")
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
    for k, _ in pairs(l.peers) do
        l.peers[k].public_data["total_points"] = 0
    end
    l = api.set_initial_data(l)
    return
end

function api.check_winner(board)
    local size = 3

    -- Check rows for 3 consecutive marks
    for row = 1, size do
        for col = 1, size - 2 do
            if board[row][col] ~= 0 and
                board[row][col] == board[row][col + 1] and
                board[row][col + 1] == board[row][col + 2] then
                return board[row][col]
            end
        end
    end

    -- Check columns for 3 consecutive marks
    for col = 1, size do
        for row = 1, size - 2 do
            if board[row][col] ~= 0 and
                board[row][col] == board[row + 1][col] and
                board[row + 1][col] == board[row + 2][col] then
                return board[row][col]
            end
        end
    end

    -- Check main diagonals (top-left to bottom-right) for 3 consecutive marks
    for row = 1, size - 2 do
        for col = 1, size - 2 do
            if board[row][col] ~= 0 and
                board[row][col] == board[row + 1][col + 1] and
                board[row + 1][col + 1] == board[row + 2][col + 2] then
                return board[row][col]
            end
        end
    end

    -- Check anti-diagonals (top-right to bottom-left) for 3 consecutive marks
    for row = 1, size - 2 do
        for col = 3, size do
            if board[row][col] ~= 0 and
                board[row][col] == board[row + 1][col - 1] and
                board[row + 1][col - 1] == board[row + 2][col - 2] then
                return board[row][col]
            end
        end
    end

    return 0
end

function api.set_piece(placementTileX, placementTileY)
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

    local winner = api.check_winner(board)
    if winner ~= 0 then
        return api.end_game(l, "won")
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
        return api.end_game(l, "draw")
    end

    l = turn.increment_turn(l, 1)
    return
end

function api.set_initial_data(l)
    l.public_data["turn_timestamp"] = system.get_time_since_epoch()
    l.public_data["game_state"] = "playing"
    local board = {
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
    }
    l.public_data["board"] = board
    l = turn.increment_dealer(l, 1)
    l.public_data["turn_idx"] = -1
    l = turn.increment_turn(l, 1)
    return l
end

function api.end_game(l, state)
    l.public_data["game_state"] = state
    if state == "won" then
        l.peers[l.calling_peer_id].public_data["total_points"] = l.peers[l.calling_peer_id].public_data["total_points"] + 1
    end
    return lobby.start_timer("_on_timer_restart_game", 1)
end

function api.on_timer_restart_game()
    local l = lobby.get()
    if l.peers[l.calling_peer_id].public_data["total_points"] >= l.tags["max_points"] and l.tags["max_points"] ~= 0 then
        l.public_data["game_state"] = "setup"
    else
        l = api.set_initial_data(l)
    end
end

return api
