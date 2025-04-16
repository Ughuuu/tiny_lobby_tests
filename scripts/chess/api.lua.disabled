-- Chess with legs (as a skin)
-- Pawnshop (as a store)
local lobby = require("lobby")
local turn = require("turn")
local math = require("math")

local api = {}

function api.start_game()
    local l = lobby.get()
    if not lobby.is_host(l.calling_peer_id) or lobby.peers_ready() ~= #l.peers or l.public_data["game_state"] ~= "setup" then
        return { error = "Cannot start game" }
    end
    l.public_data["game_state"] = "moving_piece"
    l = _increment_turn(l)
    l.sealed = true
    l = _set_initial_data(l)
    lobby.save(l)
end

function api.move_piece(fromX, fromY, toX, toY, promotionPiece)
    local l = lobby.get()
    -- Validate it's the player's turn
    local err = _validate_peer_turn(l)
    if err then
        return err
    end
    err = _validate_move(l, fromX, fromY, toX, toY)
    if err then
        return err
    end
    -- Check the player's color
    local currentTurnColor = "black"
    if l.public_data["turn_idx"] % 2 == 0 then
        currentTurnColor = "white"
    end
    local board = l.public_data["board"]
    local movingPiece = board[fromY+1][fromX+1] -- Lua uses 1-based indexing
    local pieceType = string.lower(movingPiece)
    -- Update the board
    board[toY+1][toX+1] = board[fromY+1][fromX+1]
    board[fromY+1][fromX+1] = ""
    -- Handle special cases: pawn promotion
    if pieceType == "p" and (toY == 0 or toY == 7) then
        -- Promote the pawn
        if currentTurnColor == "white" then
            board[toY+1][toX+1] = string.upper(promotionPiece)
        else
            board[toY+1][toX+1] = string.lower(promotionPiece)
        end
    end
    -- Record the move
    table.insert(l.public_data["moves"], {fromX, fromY, toX, toY, movingPiece})
    -- Update the turn
    l = _increment_turn(l)
    -- Check end game
    local otherColor = currentTurnColor == "white" and "black" or "white"
    _check_end_game(otherColor, board, l)
    -- Save the updated lobby state
    lobby.save(l)
    return nil
end

local function _validate_move(l, fromX, fromY, toX, toY)
    -- Validate coordinates are within bounds
    if fromX < 0 or fromX > 7 or fromY < 0 or fromY > 7 or toX < 0 or toX > 7 or toY < 0 or toY > 7 then
        return { error = "Coordinates out of bounds." }
    end

    local board = l.public_data["board"]
    -- Get the piece at the source position (adjusting for 1-based index)
    local movingPiece = board[fromY+1][fromX+1]
    if movingPiece == "" then
        return { error = "No piece at the source position." }
    end
    
    -- Check the player's color
    local currentTurnColor = "black"
    if l.public_data["turn_idx"] % 2 == 0 then
        currentTurnColor = "white"
    end
    if (currentTurnColor == "white" and not _is_piece_white(movingPiece)) or 
       (currentTurnColor == "black" and not _is_piece_black(movingPiece)) then
        return { error = "You can only move your own pieces." }
    end

    -- Check the destination position
    local destinationPiece = board[toY+1][toX+1]
    if destinationPiece ~= "" then
        if (currentTurnColor == "white" and _is_piece_white(destinationPiece)) or 
           (currentTurnColor == "black" and _is_piece_black(destinationPiece)) then
            return { error = "Cannot capture your own piece." }
        end
    end

    -- Determine the type of piece and validate its move
    local pieceType = string.lower(movingPiece)
    local validMove = false
    if pieceType == "p" then
        validMove = _validate_pawn_move(fromX, fromY, toX, toY, board, l)
    elseif pieceType == "n" then
        validMove = _validate_knight_move(fromX, fromY, toX, toY)
    elseif pieceType == "b" then
        validMove = _validate_bishop_move(fromX, fromY, toX, toY, board)
    elseif pieceType == "r" then
        validMove = _validate_rook_move(fromX, fromY, toX, toY, board)
    elseif pieceType == "q" then
        validMove = _validate_queen_move(fromX, fromY, toX, toY, board)
    elseif pieceType == "k" then
        validMove = _validate_king_move(fromX, fromY, toX, toY, board, l)
    end
    if not validMove then
        return { error = "Invalid move for the piece." }
    end
    -- Simulate the move and check if the king is left in check
    local simulatedBoard = _simulate_board_move(board, fromX, fromY, toX, toY)
    if _is_king_in_check(simulatedBoard, currentTurnColor, l) then
        return { error = "Move would leave your king in check." }
    end

    return nil
end

local function _validate_pawn_move(fromX, fromY, toX, toY, board, l)
    local piece = board[fromY+1][fromX+1]
    local destinationPiece = board[toY+1][toX+1]
    -- White moves up, Black moves down
    local forward = 1
    local startRow = 1
    local otherStartRow = 6
    if piece == "P" then
        forward = -1
        startRow = 6
        otherStartRow = 1
    end
    
    local diffX = math.abs(fromX - toX)
    local diffY = toY - fromY

    -- En Passant
    if diffX == 1 and diffY == forward and destinationPiece == "" then
        -- Check if the move is valid for en passant
        -- Capture the opposing pawn if it moved two squares forward last turn
        if #l.public_data["moves"] ~= 0 then
            local lastMove = l.public_data["moves"][#l.public_data["moves"]]
            local lastFromX = lastMove[1]
            local lastFromY = lastMove[2]
            local lastToX = lastMove[3]
            local lastToY = lastMove[4]
            local lastPieceMoved = lastMove[5]
            -- if last move was not two squares forward from a pawn at start row, ignore this case
            -- check if last move was a pawn also
            if math.abs(lastToY - lastFromY) == 2 and lastToX == lastFromX and lastFromY == otherStartRow and string.lower(lastPieceMoved) == "p" then
                -- check if en passant is valid
                if math.abs(toY - otherStartRow) == 1 and board[fromY+1+forward][toX+1] == "" then
                    board[lastToY+1][lastToX+1] = ""
                    return true  -- Successful en passant capture
                end
            end
        end
    end

    if destinationPiece == "" then -- Normal move
        return (diffX == 0 and diffY == forward) or 
               (diffX == 0 and diffY == 2 * forward and fromY == startRow and board[fromY+1 + forward][fromX+1] == "")
    else -- Capture
        return (diffX == 1 and diffY == forward)
    end
end

local function _validate_knight_move(fromX, fromY, toX, toY)
    local diffX = math.abs(fromX - toX)
    local diffY = math.abs(fromY - toY)
    return (diffX == 2 and diffY == 1) or (diffX == 1 and diffY == 2)
end

local function _validate_bishop_move(fromX, fromY, toX, toY, board)
    if math.abs(fromX - toX) ~= math.abs(fromY - toY) then
        return false
    end
    local stepX = -1
    local stepY = -1
    if fromX < toX then
        stepX = 1
    end
    if fromY < toY then
        stepY = 1
    end
    for i = 1, math.abs(fromX - toX) - 1 do
        if board[fromY+1 + i * stepY][fromX+1 + i * stepX] ~= "" then
            return false
        end
    end
    return true
end

local function _validate_rook_move(fromX, fromY, toX, toY, board)
    if fromX ~= toX and fromY ~= toY then
        return false
    end
    local stepX = -1
    local stepY = -1
    if fromX == toX then
        stepX = 0
    elseif fromX < toX then
        stepX = 1
    end
    if fromY == toY then
        stepY = 0
    elseif fromY < toY then
        stepY = 1
    end
    local maxSteps = math.max(math.abs(fromX - toX), math.abs(fromY - toY))
    for i = 1, maxSteps - 1 do
        if board[fromY+1 + i * stepY][fromX+1 + i * stepX] ~= "" then
            return false
        end
    end
    return true
end

local function _validate_queen_move(fromX, fromY, toX, toY, board)
    return _validate_bishop_move(fromX, fromY, toX, toY, board) or _validate_rook_move(fromX, fromY, toX, toY, board)
end

local function _validate_king_move(fromX, fromY, toX, toY, board, l)
    local piece = board[fromY+1][fromX+1]
    local pieceColor = "white"
    if _is_piece_black(piece) then
        pieceColor = "black"
    end
    if math.abs(fromX - toX) <= 1 and math.abs(fromY - toY) <= 1 then
        return true -- Normal king move
    end

    -- Castling
    if math.abs(fromX - toX) == 2 and fromY == toY then
        local rookX = 0
        if toX > fromX then
            rookX = 7
        end
        local rookY = fromY
        local rookPiece = board[rookY+1][rookX+1]
        if string.lower(rookPiece) ~= "r" then
            return false
        end
        -- Check that the king has not moved this game
        for i = 1, #l.public_data["moves"] do
            local historicMove = l.public_data["moves"][i]
            local historicMovePiece = historicMove[5]
            -- if you moved the king before
            if historicMovePiece == piece then
                return false
            end
        end
        -- Ensure squares between king and rook are empty and not under attack
        local stepX = -1
        if toX > fromX then
            stepX = 1
        end
        for i = 1, 2 do
            if board[fromY+1][fromX+1 + i * stepX] ~= "" or _is_square_attacked(board, fromX + i * stepX, fromY, pieceColor, l) then
                return false
            end
        end
        return true
    end
    return false
end

local function _is_king_in_check(board, color, l)
    local kingPosition = _find_king_position(board, color)
    local otherColor = color == "white" and "black" or "white"
    return _is_square_attacked(board, kingPosition[1], kingPosition[2], otherColor, l)
end

local function _is_square_attacked(board, x, y, attackingColor, l)
    for fromY = 0, 7 do
        for fromX = 0, 7 do
            local piece = board[fromY+1][fromX+1]
            if piece == "" then
                goto continue
            end

            -- Determine the color of the piece
            local pieceColor = "white"
            if piece == string.lower(piece) then
                pieceColor = "black"
            end

            -- Skip pieces of the same color as the square being checked
            if pieceColor ~= attackingColor then
                goto continue
            end

            -- Check if this piece attacks the square (x, y)
            local pieceType = string.lower(piece)
            if pieceType == "p" then -- Pawn
                if _validate_pawn_move(fromX, fromY, x, y, board, l) then
                    return true
                end
            elseif pieceType == "n" then -- Knight
                if _validate_knight_move(fromX, fromY, x, y) then
                    return true
                end
            elseif pieceType == "b" then -- Bishop
                if _validate_bishop_move(fromX, fromY, x, y, board) then
                    return true
                end
            elseif pieceType == "r" then -- Rook
                if _validate_rook_move(fromX, fromY, x, y, board) then
                    return true
                end
            elseif pieceType == "q" then -- Queen
                if _validate_queen_move(fromX, fromY, x, y, board) then
                    return true
                end
            elseif pieceType == "k" then -- King
                if _validate_king_move(fromX, fromY, x, y, board, l) then
                    return true
                end
            end
            ::continue::
        end
    end
    return false
end

local function _find_king_position(board, color)
    for y = 0, 7 do
        for x = 0, 7 do
            if (color == "white" and board[y+1][x+1] == "K") or (color == "black" and board[y+1][x+1] == "k") then
                return {x, y}
            end
        end
    end
    return nil
end

local function _simulate_board_move(board, fromX, fromY, toX, toY)
    -- Create a deep copy of the board to simulate the move
    local simulatedBoard = {}
    for y = 1, 8 do
        simulatedBoard[y] = {}
        for x = 1, 8 do
            simulatedBoard[y][x] = board[y][x]
        end
    end

    -- Perform the move on the simulated board (adjusting for 1-based indexing)
    local movingPiece = simulatedBoard[fromY+1][fromX+1]
    simulatedBoard[toY+1][toX+1] = movingPiece
    simulatedBoard[fromY+1][fromX+1] = ""

    return simulatedBoard
end

local function _check_end_game(turnColor, board, l)
    -- Check if the other player is in check
    local inCheck = _is_king_in_check(board, turnColor, l)

    -- Check if there are any valid moves for the current player
    local hasValidMoves = false
    for y = 0, 7 do
        for x = 0, 7 do
            local piece = board[y+1][x+1]
            if piece == "" then
                goto continue_piece
            end
            -- Skip pieces that don't belong to the current player
            if (turnColor == "white" and _is_piece_black(piece)) or 
               (turnColor == "black" and _is_piece_white(piece)) then
                goto continue_piece
            end
            for toY = 0, 7 do
                for toX = 0, 7 do
                    -- Simulate each possible move
                    if _validate_move(l, x, y, toX, toY) == nil then
                        hasValidMoves = true
                        break
                    end
                end
                if hasValidMoves then
                    break
                end
            end
            if hasValidMoves then
                break
            end
            ::continue_piece::
        end
        if hasValidMoves then
            break
        end
    end

    if not hasValidMoves then
        if inCheck then
            -- Checkmate
            l.public_data["game_state"] = "checkmate"
            l.public_data["winner"] = turnColor == "white" and "black" or "white"
        else
            -- Stalemate
            l.public_data["game_state"] = "stalemate"
        end
        return
    end

    -- Check for draw by insufficient material
    if _is_insufficient_material(board) then
        l.public_data["game_state"] = "draw_insufficient_material"
        return
    end

    -- Draw by repetition (not implemented here, but you would check l.public_data["moves"])
    return
end

local function _is_insufficient_material(board)
    local pieces = {}
    local pieceCount = {}
    for _, row in ipairs(board) do
        for _, piece in ipairs(row) do
            if piece ~= "" then
                local lowerPiece = string.lower(piece)
                table.insert(pieces, lowerPiece)
                pieceCount[lowerPiece] = (pieceCount[lowerPiece] or 0) + 1
            end
        end
    end
    -- Only kings left
    if #pieces == 2 then
        return true
    end
    -- King and bishop or knight vs king
    if #pieces == 3 then
        if pieceCount["b"] or pieceCount["n"] then 
            return true
        end
    end
    return false
end

local function _validate_peer_turn(l)
    if l.public_data["turn"] ~= l.calling_peer_id then
        return { error = "Not your turn" }
    end
    return nil
end

local function _set_initial_data(l)
    local dealerIdx = l.public_data["dealer_idx"]
    -- P pawns, R rooks, N knights, B bishops, Q queen, K king
    -- uppercase
    l.public_data["board"] = {
        {"r", "n", "b", "q", "k", "b", "n", "r"}, -- Black pieces
        {"p", "p", "p", "p", "p", "p", "p", "p"}, -- Black pawns
        {"", "", "", "", "", "", "", ""},
        {"", "", "", "", "", "", "", ""},
        {"", "", "", "", "", "", "", ""},
        {"", "", "", "", "", "", "", ""},
        {"P", "P", "P", "P", "P", "P", "P", "P"}, -- White pawns
        {"R", "N", "B", "Q", "K", "B", "N", "R"}  -- White pieces
    }
    l.public_data["moves"] = {}
    return l
end

local function _is_piece_white(piece)
    return string.upper(piece) == piece
end

local function _is_piece_black(piece)
    return string.lower(piece) == piece
end

local function _increment_dealer(l)
    l.public_data["dealer_idx"] = l.public_data["dealer_idx"] + 1
    l.public_data["dealer"] = l.peers[lobby.peers_ordered()[l.public_data["dealer_idx"] % #l.peers + 1]].id
    return l
end

local function _increment_turn(l)
    l.public_data["turn_idx"] = l.public_data["turn_idx"] + 1
    l.public_data["turn"] = l.peers[lobby.peers_ordered()[l.public_data["turn_idx"] % #l.peers + 1]].id
    return l
end

return api
