local lobby = require("lobby")
local system = require("system")
local api = {}

-- Tetris constants
api.GRID_WIDTH = 10
api.GRID_HEIGHT = 20
api.PIECE_TYPES = {"I", "J", "L", "O", "S", "T", "Z"}
api.LOCK_DELAY = 0.5 -- 500ms lock delay
api.ARE = 0.5 -- 500ms entry delay (optional)

-- Piece definitions
api.PIECE_SHAPES = {
    I = {
        {0,0,0,0},
        {1,1,1,1},
        {0,0,0,0},
        {0,0,0,0}
    },
    J = {
        {2,0,0},
        {2,2,2},
        {0,0,0}
    },
    L = {
        {0,0,3},
        {3,3,3},
        {0,0,0}
    },
    O = {
        {4,4},
        {4,4}
    },
    S = {
        {0,5,5},
        {5,5,0},
        {0,0,0}
    },
    T = {
        {0,6,0},
        {6,6,6},
        {0,0,0}
    },
    Z = {
        {7,7,0},
        {0,7,7},
        {0,0,0}
    }
}

-- Scoring system
api.SCORE_VALUES = {
    single = 100,
    double = 300,
    triple = 500,
    tetris = 800,
    soft_drop = 1,
    hard_drop = 2,
    t_spin_mini = 100,
    t_spin = 400,
    t_spin_mini_double = 200,
    t_spin_double = 1200,
    t_spin_triple = 1600
}

function api.start_game()
    local l = lobby.get()
    if l.peers[l.calling_peer_id].id ~= l.host then
        return { error = "You are not the host" }
    end
    if l.public_data["game_state"] ~= "setup" then
        return { error = "Game already started" }
    end
    
    l.public_data["game_state"] = "playing"
    l.public_data["start_time"] = system.get_time()
    
    -- Initialize each player's game
    for peer_id, peer in pairs(l.peers) do
        peer.public_data["game"] = {
            board = api.create_empty_board(),
            current_piece = api.generate_new_piece(),
            next_piece = api.generate_new_piece(),
            held_piece = nil,
            can_hold = true,
            piece_position = {x = 3, y = -2}, -- Start above the board
            score = 0,
            level = 1,
            lines_cleared = 0,
            last_drop_time = system.get_time(),
            last_move_time = system.get_time(),
            lock_pending = false,
            lock_start_time = nil,
            game_over = false,
            entry_delay = api.ARE,
            last_t_spin = nil
        }
        peer.public_data["points"] = 0
        peer.public_data["total_points"] = 0
    end
    
    return true
end

function api.create_empty_board()
    local board = {}
    for y = 1, api.GRID_HEIGHT do
        board[y] = {}
        for x = 1, api.GRID_WIDTH do
            board[y][x] = 0
        end
    end
    return board
end

function api.generate_new_piece()
    local piece_type = api.PIECE_TYPES[math.random(#api.PIECE_TYPES)]
    return {
        type = piece_type,
        shape = api.deep_copy(api.PIECE_SHAPES[piece_type]),
        rotation = 0
    }
end

function api.deep_copy(t)
    if type(t) ~= "table" then return t end
    local copy = {}
    for k, v in pairs(t) do
        copy[k] = api.deep_copy(v)
    end
    return copy
end

function api.move_piece(direction)
    local l = lobby.get()
    if l.public_data["game_state"] ~= "playing" then
        return { error = "Game not in progress" }
    end
    
    local peer_game = l.peers[l.calling_peer_id].public_data.game
    if peer_game.game_over then
        return { error = "Your game is over" }
    end
    
    -- Handle entry delay
    if peer_game.entry_delay > 0 then
        peer_game.entry_delay = peer_game.entry_delay - system.get_delta_time()
        return { error = "Piece is entering" }
    end
    
    local new_pos = {x = peer_game.piece_position.x, y = peer_game.piece_position.y}
    
    if direction == "left" then
        new_pos.x = new_pos.x - 1
    elseif direction == "right" then
        new_pos.x = new_pos.x + 1
    elseif direction == "down" then
        new_pos.y = new_pos.y + 1
    end
    
    local can_move = api.is_valid_position(peer_game.current_piece, new_pos, peer_game.board)
    
    if can_move then
        peer_game.piece_position = new_pos
        peer_game.last_move_time = system.get_time()
        
        -- Cancel any pending lock
        peer_game.lock_pending = false
        peer_game.lock_start_time = nil
        
        if direction == "down" then
            peer_game.last_drop_time = system.get_time()
            peer_game.score = peer_game.score + api.SCORE_VALUES.soft_drop
        end
        
        return true
    elseif direction == "down" then
        -- Start lock delay when piece can't move down
        if not peer_game.lock_pending then
            peer_game.lock_pending = true
            peer_game.lock_start_time = system.get_time()
        end
    end
    
    return false, "Invalid move"
end

function api.hard_drop()
    local l = lobby.get()
    if l.public_data["game_state"] ~= "playing" then
        return { error = "Game not in progress" }
    end
    
    local peer_game = l.peers[l.calling_peer_id].public_data.game
    if peer_game.game_over then
        return { error = "Your game is over" }
    end
    
    local drop_distance = 0
    local test_pos = api.deep_copy(peer_game.piece_position)
    
    -- Calculate how far we can drop
    while api.is_valid_position(peer_game.current_piece, test_pos, peer_game.board) do
        test_pos.y = test_pos.y + 1
        drop_distance = drop_distance + 1
    end
    
    if drop_distance > 0 then
        peer_game.piece_position.y = peer_game.piece_position.y + (drop_distance - 1)
        peer_game.score = peer_game.score + (api.SCORE_VALUES.hard_drop * drop_distance)
        peer_game.last_move_time = system.get_time()
        api.lock_piece(l.calling_peer_id)
        return true
    end
    
    return false, "Cannot drop"
end

function api.rotate_piece(direction)
    local l = lobby.get()
    if l.public_data["game_state"] ~= "playing" then
        return { error = "Game not in progress" }
    end
    
    local peer_game = l.peers[l.calling_peer_id].public_data.game
    if peer_game.game_over then
        return { error = "Your game is over" }
    end
    
    local piece = peer_game.current_piece
    local rotated = api.deep_copy(piece.shape)
    
    -- Rotate based on direction
    if direction == "right" then
        -- Clockwise rotation
        for i = 1, #piece.shape do
            for j = 1, #piece.shape[1] do
                rotated[j][#piece.shape - i + 1] = piece.shape[i][j]
            end
        end
    else
        -- Counter-clockwise rotation
        for i = 1, #piece.shape do
            for j = 1, #piece.shape[1] do
                rotated[#piece.shape[1] - j + 1][i] = piece.shape[i][j]
            end
        end
    end
    
    -- Check if rotation is valid
    local test_piece = {
        type = piece.type,
        shape = rotated,
        rotation = (piece.rotation + (direction == "right" and 1 or -1)) % 4
    }
    
    if api.is_valid_position(test_piece, peer_game.piece_position, peer_game.board) then
        peer_game.current_piece = test_piece
        peer_game.last_move_time = system.get_time()
        
        -- Check for T-Spin
        if piece.type == "T" then
            peer_game.last_t_spin = api.check_t_spin(peer_game)
        end
        
        -- Cancel any pending lock
        peer_game.lock_pending = false
        peer_game.lock_start_time = nil
        return true
    else
        -- Try wall kicks
        local kicks = {
            {x = -1, y = 0}, {x = 1, y = 0},
            {x = 0, y = -1}, {x = -2, y = 0}, {x = 2, y = 0}
        }
        
        for _, kick in ipairs(kicks) do
            local test_pos = {
                x = peer_game.piece_position.x + kick.x,
                y = peer_game.piece_position.y + kick.y
            }
            
            if api.is_valid_position(test_piece, test_pos, peer_game.board) then
                peer_game.current_piece = test_piece
                peer_game.piece_position = test_pos
                peer_game.last_move_time = system.get_time()
                
                -- Check for T-Spin
                if piece.type == "T" then
                    peer_game.last_t_spin = api.check_t_spin(peer_game)
                end
                
                -- Cancel any pending lock
                peer_game.lock_pending = false
                peer_game.lock_start_time = nil
                return true
            end
        end
    end
    
    return false, "Cannot rotate"
end

function api.check_t_spin(peer_game)
    -- Simplified T-Spin detection
    local corners_occupied = 0
    local pos = peer_game.piece_position
    local board = peer_game.board
    
    -- Check the 4 corners around T piece
    if pos.x > 1 and pos.y > 1 and board[pos.y-1][pos.x-1] ~= 0 then corners_occupied = corners_occupied + 1 end
    if pos.x < api.GRID_WIDTH and pos.y > 1 and board[pos.y-1][pos.x+1] ~= 0 then corners_occupied = corners_occupied + 1 end
    if pos.x > 1 and pos.y < api.GRID_HEIGHT and board[pos.y+1][pos.x-1] ~= 0 then corners_occupied = corners_occupied + 1 end
    if pos.x < api.GRID_WIDTH and pos.y < api.GRID_HEIGHT and board[pos.y+1][pos.x+1] ~= 0 then corners_occupied = corners_occupied + 1 end
    
    if corners_occupied >= 3 then
        return "full"
    elseif corners_occupied >= 2 then
        return "mini"
    end
    return nil
end

function api.hold_piece()
    local l = lobby.get()
    if l.public_data["game_state"] ~= "playing" then
        return { error = "Game not in progress" }
    end
    
    local peer_game = l.peers[l.calling_peer_id].public_data.game
    if peer_game.game_over then
        return { error = "Your game is over" }
    end
    
    if not peer_game.can_hold then
        return { error = "Cannot hold now" }
    end
    
    -- Handle entry delay
    if peer_game.entry_delay > 0 then
        return { error = "Cannot hold during entry delay" }
    end
    
    -- Swap current piece with held piece
    local current = peer_game.current_piece
    local held = peer_game.held_piece
    
    peer_game.held_piece = current
    peer_game.current_piece = held or api.generate_new_piece()
    peer_game.piece_position = {x = 3, y = -2}
    peer_game.can_hold = false
    peer_game.entry_delay = api.ARE
    peer_game.last_move_time = system.get_time()
    
    -- Cancel any pending lock
    peer_game.lock_pending = false
    peer_game.lock_start_time = nil
    
    if not held then
        peer_game.next_piece = api.generate_new_piece()
    end
    
    return true
end

function api.is_valid_position(piece, position, board)
    for y = 1, #piece.shape do
        for x = 1, #piece.shape[y] do
            if piece.shape[y][x] ~= 0 then
                local board_x = position.x + x - 1
                local board_y = position.y + y - 1
                
                if board_x < 1 or board_x > api.GRID_WIDTH or board_y > api.GRID_HEIGHT then
                    return false
                end
                
                if board_y >= 1 and board[board_y][board_x] ~= 0 then
                    return false
                end
            end
        end
    end
    return true
end

function api.lock_piece(peer_id)
    local l = lobby.get()
    local peer_game = l.peers[peer_id].public_data.game
    local piece = peer_game.current_piece
    local pos = peer_game.piece_position
    
    -- Add piece to the board
    for y = 1, #piece.shape do
        for x = 1, #piece.shape[y] do
            if piece.shape[y][x] ~= 0 then
                local board_y = pos.y + y - 1
                if board_y >= 1 then
                    peer_game.board[board_y][pos.x + x - 1] = piece.shape[y][x]
                end
            end
        end
    end
    
    -- Check for game over (piece locked above visible area)
    if pos.y <= 0 then
        peer_game.game_over = true
        api.check_all_players_finished()
        return
    end
    
    -- Clear completed lines and calculate score
    local lines_cleared = api.clear_lines(peer_id)
    api.update_score(peer_id, lines_cleared)
    
    -- Spawn new piece
    peer_game.current_piece = peer_game.next_piece
    peer_game.next_piece = api.generate_new_piece()
    peer_game.piece_position = {x = 3, y = -2}
    peer_game.can_hold = true
    peer_game.entry_delay = api.ARE
    peer_game.last_drop_time = system.get_time()
    peer_game.lock_pending = false
    peer_game.lock_start_time = nil
    
    -- Check if new piece can be placed
    if not api.is_valid_position(peer_game.current_piece, peer_game.piece_position, peer_game.board) then
        peer_game.game_over = true
        api.check_all_players_finished()
    end
end

function api.clear_lines(peer_id)
    local peer_game = lobby.get().peers[peer_id].public_data.game
    local new_board = api.create_empty_board()
    local lines_cleared = 0
    local new_row = api.GRID_HEIGHT
    
    for y = api.GRID_HEIGHT, 1, -1 do
        local line_complete = true
        for x = 1, api.GRID_WIDTH do
            if peer_game.board[y][x] == 0 then
                line_complete = false
                break
            end
        end
        
        if not line_complete then
            for x = 1, api.GRID_WIDTH do
                new_board[new_row][x] = peer_game.board[y][x]
            end
            new_row = new_row - 1
        else
            lines_cleared = lines_cleared + 1
        end
    end
    
    peer_game.board = new_board
    return lines_cleared
end

function api.update_score(peer_id, lines_cleared)
    local peer_game = lobby.get().peers[peer_id].public_data.game
    
    if lines_cleared > 0 then
        local score_add = 0
        
        -- Handle T-Spin scoring
        if peer_game.last_t_spin then
            if peer_game.last_t_spin == "mini" then
                if lines_cleared == 1 then score_add = api.SCORE_VALUES.t_spin_mini
                elseif lines_cleared == 2 then score_add = api.SCORE_VALUES.t_spin_mini_double end
            else
                if lines_cleared == 1 then score_add = api.SCORE_VALUES.t_spin
                elseif lines_cleared == 2 then score_add = api.SCORE_VALUES.t_spin_double
                elseif lines_cleared == 3 then score_add = api.SCORE_VALUES.t_spin_triple end
            end
        else
            -- Normal line clears
            if lines_cleared == 1 then score_add = api.SCORE_VALUES.single
            elseif lines_cleared == 2 then score_add = api.SCORE_VALUES.double
            elseif lines_cleared == 3 then score_add = api.SCORE_VALUES.triple
            elseif lines_cleared == 4 then score_add = api.SCORE_VALUES.tetris end
        end
        
        -- Apply level multiplier
        score_add = score_add * peer_game.level
        
        peer_game.score = peer_game.score + score_add
        peer_game.lines_cleared = peer_game.lines_cleared + lines_cleared
        
        -- Level up every 10 lines
        peer_game.level = math.floor(peer_game.lines_cleared / 10) + 1
        
        -- Reset T-Spin state
        peer_game.last_t_spin = nil
    end
end

function api.check_all_players_finished()
    local l = lobby.get()
    local all_finished = true
    local highest_score = 0
    local winner = nil
    
    for peer_id, peer in pairs(l.peers) do
        if not peer.public_data.game.game_over then
            all_finished = false
        elseif peer.public_data.game.score > highest_score then
            highest_score = peer.public_data.game.score
            winner = peer_id
        end
    end
    
    if all_finished then
        if winner then
            l.peers[winner].public_data.total_points = l.peers[winner].public_data.total_points + 1
        end
        lobby.start_timer("_on_timer_restart_game", 3)
    end
end

function api.on_timer_restart_game()
    local l = lobby.get()
    local max_points = l.tags["max_points"] or 5
    local game_over = false
    
    for _, peer in pairs(l.peers) do
        if peer.public_data.total_points >= max_points then
            game_over = true
            break
        end
    end
    
    if game_over then
        l.public_data["game_state"] = "setup"
    else
        api.start_game()
    end
end

return api
