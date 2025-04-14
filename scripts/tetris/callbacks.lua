local api = require("api")
local lobby = require("lobby")
local system = require("system")

local callbacks = {}

function callbacks.on_create(minPlayers, maxPlayers)
    local l = lobby.get()
    if l.max_players < minPlayers or l.max_players > maxPlayers then
        return { error = ("Players must be between "..minPlayers.." and "..maxPlayers) }
    end

    l.tags["max_points"] = l.tags["max_points"] or 5
    l.public_data["start_time"] = system.get_time_since_epoch()
    l.public_data["game_state"] = "setup"
    return
end

function callbacks.on_join()
    local l = lobby.get()
    if l.public_data["game_state"] == "setup" then return end
    -- If a player joins, add initial data
    local peer = l.peers[l.calling_peer_id]
    api.set_peer_initial_data(peer)
    return
end

function callbacks.on_left()
    local l = lobby.get()
    if l.public_data["game_state"] == "setup" then return end
    
    -- If a player leaves, check if game should end
    local remaining_players = 0
    for _ in pairs(l.peers) do remaining_players = remaining_players + 1 end
    
    if remaining_players < 1 then
        l.public_data["game_state"] = "setup"
    else
        api.check_all_players_finished()
    end
end

function callbacks.on_tags(tags)
    local l = lobby.get()
    if l.public_data["game_state"] ~= "setup" then
        return { error = "Cannot change settings during game" }
    end
    return
end

function callbacks.on_ready(ready)
    local l = lobby.get()
    if l.public_data["game_state"] ~= "setup" then
        return { error = "Game already started" }
    end
    
    -- Check if all players are ready to start
    local all_ready = true
    for _, peer in pairs(l.peers) do
        if not peer.ready then
            all_ready = false
            break
        end
    end
    
    if all_ready then
        api.start_game()
    end
    return
end

function callbacks.on_tick()
    local l = lobby.get()
    if l.public_data["game_state"] ~= "playing" then return end
    
    local current_time = system.get_time_since_epoch()
    local delta_time = l.tick_rate
    
    -- Update each player's game
    for peer_id, peer in pairs(l.peers) do
        local game = peer.public_data
        if not game.game_over then
            -- Handle entry delay
            if game.entry_delay > 0 then
                game.entry_delay = game.entry_delay - delta_time
                if game.entry_delay <= 0 then
                    game.entry_delay = 0
                    game.last_drop_time = current_time
                end
            else
                -- Handle lock delay
                if game.lock_pending and current_time - game.lock_start_time >= api.LOCK_DELAY then
                    api.lock_piece(peer_id)
                end
                
                -- Auto-drop
                local drop_interval = math.max(0.05, 1.0 - (game.level * 0.05))
                if current_time - game.last_drop_time >= drop_interval then
                    api.move_piece("down")
                end
            end
        end
    end
end

return callbacks
