local turn = require("turn")
local lobby = require("lobby")

local api = {}

local subfolder_image_counts = {
    birds = 7,    -- res://game/card_front_images/birds has 7 images
    bugs = 16,
    flowers = 27, 
}

local available_subfolders = {}
local current_subfolder = ""

function api.start_game()
    local l = lobby.get()
    if l.calling_peer_id ~= l.host then
        return { error = "You are not the host" }
    end
    if l.public_data["game_state"] ~= "setup" then
        return { error = "Game already started" }
    end
    l.public_data["turns_played"] = 0
    l = api.set_initial_data(l)
    return l
end

function api.set_initial_data(l)
    for peer_id, _ in pairs(l.peers) do
        l.peers[peer_id].public_data["points"] = 0
    end
    l.public_data["image_folder"] = api.random_image_folder(l)
    l = api.new_game_data(l)
    return l
end

function api.new_game_data(l)
    local rows = l.tags["grid_rows"]
    local cols = l.tags["grid_cols"]

    l.public_data["grid_size"] = { rows = rows, cols = cols }
    current_subfolder = l.public_data["image_folder"]
    l.public_data["game_state"] = "playing"
    l.public_data["flipped"] = {{x=0, y=0}, {x=0, y=0}}
    l.public_data["flipped_values"] = {"0", "0"}
    l.private_data["grid"], l.public_data["revealed"] = api.create_grid(rows, cols)
    -- l.private_data["grid"], l.public_data["revealed"] = api.test_grid(rows, cols)
    l.public_data["winner"] = nil
    l.public_data["turn_idx"] = -1
    l = turn.increment_dealer(l, 1)
    l = turn.increment_turn(l, 1)
    return l
end

function api.random_image_folder(l)
    local grid_size = l.tags["grid_rows"] * l.tags["grid_cols"]
    local subfolder_keys = {}
    for subfolder, _ in pairs(subfolder_image_counts) do
        table.insert(subfolder_keys, subfolder)
    end

    if #available_subfolders == 0 then
        for _, subfolder in ipairs(subfolder_keys) do
            table.insert(available_subfolders, subfolder)
        end
    end

    print("Available subfolders: " .. table.concat(available_subfolders, ", "))

    local valid_subfolders = {}
    for _, subfolder in ipairs(available_subfolders) do
        if subfolder_image_counts[subfolder] >= grid_size then
            table.insert(valid_subfolders, subfolder)
        end
    end

    if #valid_subfolders == 0 then
        available_subfolders = {}
        for _, subfolder in ipairs(subfolder_keys) do
            table.insert(available_subfolders, subfolder)
        end
        for _, subfolder in ipairs(available_subfolders) do
            if subfolder_image_counts[subfolder] >= grid_size then
                table.insert(valid_subfolders, subfolder)
            end
        end
    end

    if #valid_subfolders == 0 then
        print("No subfolder with enough cards for grid size: " .. grid_size)
        return subfolder_keys[1]  -- Fallback
    end

    local random_idx = math.random(1, #valid_subfolders)
    local selected_subfolder = valid_subfolders[random_idx]

    for i, subfolder in ipairs(available_subfolders) do
        if subfolder == selected_subfolder then
            table.remove(available_subfolders, i)
            break
        end
    end

    print("Selected subfolder: " .. selected_subfolder .. ", Cards: " .. subfolder_image_counts[selected_subfolder])
    return selected_subfolder
end

function api.create_grid(rows, cols)
    local total_cards = rows * cols
    if total_cards % 2 ~= 0 then
        return { error = "Grid size must be even." }
    end
    local pairs_length = total_cards / 2

    local cards_nr = subfolder_image_counts[current_subfolder]
    local available_values = {}
    for i = 1, cards_nr do
        table.insert(available_values, tostring(i))
    end

    for _ = 1, 10 do
        for i = #available_values, 2, -1 do
            local j = math.random(i)
            available_values[i], available_values[j] = available_values[j], available_values[i]
        end
    end

    local cards = {}
    for i = 1, pairs_length do
        local value = available_values[i]
        cards[i * 2 - 1] = value
        cards[i * 2] = value
    end

    for _ = 1, 10 do
        for i = #cards, 2, -1 do
            local j = math.random(i)
            cards[i], cards[j] = cards[j], cards[i]
        end
    end

    local grid = {}
    local empty_grid = {}
    local idx = 1
    for x = 1, rows do
        grid[x] = {}
        empty_grid[x] = {}
        for y = 1, cols do
            grid[x][y] = cards[idx]
            empty_grid[x][y] = 0
            idx = idx + 1
        end
    end
    return grid, empty_grid
end

function api.flip_card(x, y)
    local l = lobby.get()
    local rows = l.tags["grid_rows"]
    local cols = l.tags["grid_cols"]
    if l.public_data["game_state"] == "flipping" then
        return { error = "Already flipping a card." }
    end
    if l.public_data["game_state"] ~= "playing" then
        return { error = "Game has not started." }
    end
    if l.public_data["turn"] ~= l.calling_peer_id then
        return { error = "Not your turn." }
    end
    if x < 1 or x > rows or y < 1 or y > cols then
        return { error = "Invalid card position. x: " .. x .. ", y: " .. y .. "." }
    end
    local revealed = l.public_data["revealed"]
    if revealed[x][y] ~= 0 then
        return { error = "Card already revealed." }
    end
    local flipped = l.public_data["flipped"]
    if flipped[1].x == x and flipped[1].y == y then
        return { error = "Card already flipped this turn." }
    end
    
    l.public_data["game_state"] = "flipping"
    
    local grid = l.private_data["grid"]
    local flipped_values = l.public_data["flipped_values"]
    if flipped[1].x == 0 and flipped[1].y == 0 then
        flipped[1] = {x = x, y = y}
        flipped_values[1] = grid[x][y]
        l.public_data["flipped"] = flipped
    elseif flipped[2].x == 0 and flipped[2].y == 0 then
        flipped[2] = {x = x, y = y}
        flipped_values[2] = grid[x][y]
        l.public_data["flipped"] = flipped
        l = api.resolve_turn(l)
    end
    l.public_data["flipped_values"] = flipped_values
    l.start_timer("_on_timer_flipped_card", 1)
    
    return l
end

function api.resolve_turn(l)
    local flipped = l.public_data["flipped"]
    local grid = l.private_data["grid"]
    local revealed = l.public_data["revealed"]
    local peer_id = l.calling_peer_id
    local rows = l.tags["grid_rows"]
    local cols = l.tags["grid_cols"]
    
    local card1_value = grid[flipped[1].x][flipped[1].y]
    local card2_value = grid[flipped[2].x][flipped[2].y]
    
    if card1_value == card2_value then
        revealed[flipped[1].x][flipped[1].y] = card1_value
        revealed[flipped[2].x][flipped[2].y] = card2_value
        l.peers[peer_id].public_data["points"] += 1
        
        local revealed_count = 0
        for x = 1, rows do
            for y = 1, cols do
                if revealed[x][y] ~= 0 then
                    revealed_count = revealed_count + 1
                end
            end
        end
        if revealed_count == rows*cols then
            l = api.end_game(l)
        end
    else
        l = turn.increment_turn(l, 1)
    end
    
    l.public_data["revealed"] = revealed
    return l
end

function api.end_game(l)
    local winner = nil
    local max_points = -1
    
    for peer_id, peer in pairs(l.peers) do
        if peer.public_data["points"] > max_points then
            max_points = peer.public_data["points"]
            winner = peer_id
        end
    end
    l.public_data["turns_played"] = l.public_data["turns_played"] + 1
    if winner then
        l.public_data["winner"] = winner
    end
    
    if l.public_data["turns_played"] < l.tags["max_turns"] then
        l.start_timer("_on_timer_new_game", 1)
    else
        l.start_timer("_on_timer_end_game", 2)
    end
    return l
end

function api._on_timer_flipped_card()
    local l = lobby.get()
    l.public_data["game_state"] = "playing"
    local flipped_values = l.public_data["flipped_values"]
    local flipped = l.public_data["flipped"]
    if flipped[2].x ~= 0 or flipped[2].y ~= 0 then
        flipped = {{x=0, y=0}, {x=0, y=0}}
        flipped_values = {"0", "0"}
        l.public_data["flipped"] = flipped
        l.public_data["flipped_values"] = flipped_values
    end
    return l
end

function api.on_timer_end_game()
    local l = lobby.get()
    l.public_data["game_state"] = "finished"
    l.start_timer("_on_timer_setup_game", 2)
    return l
end

function api.on_timer_setup_game()
    local l = lobby.get()
    l.public_data["game_state"] = "setup"
    return l
end

function api._on_timer_set_new_game_data()
    local l = lobby.get()
    l = api.new_game_data(l)
    return l
end

function api.on_timer_new_game()
    local l = lobby.get()
    current_subfolder = l.public_data["image_folder"]
    l.public_data["game_state"] = "new_game"
    l.public_data["image_folder"] = api.random_image_folder(l)
    l.start_timer("_on_timer_set_new_game_data", 2)
    return l
end

function api.get_test_grid()
    local grid = {
        {1, 2, 1},
        {3, 3, 2},
    }
    local empty_grid = {
        {0, 0, 0},
        {0, 0, 0},
    }
    return grid, empty_grid
end

return api

