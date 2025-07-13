local lobby = require("lobby")
local main = {}

function main.turn(dir: number)
    if type(dir) ~= "number" then
        error("dir must be numbers")
    end
    if dir < 0 or dir > 3 then
        error("invalid dir")
    end
    local l = lobby.get()
    local peer = l.peers[l.calling_peer_id]
    peer.public_data["dir"] = dir
end

function main.move_press(dir: number)
    local l = lobby.get()
    local peer = l.peers[l.calling_peer_id]
    if dir < 0 or dir > 3 then
        error("invalid dir")
    end

    local move_start_ms = peer.public_data:get_int("move_start")
    local move_time_ms = peer.public_data:get_int("move_time")
    local current_time_ms = get_ticks_ms()

    if move_start_ms + move_time_ms > current_time_ms then
        return
    end

    if peer.public_data:get_bool("is_moving") and dir == peer.public_data:get_int("dir") then
        return
    end

    main._move_peer(peer, l, dir, current_time_ms)
end

function main.move_release()
    local l = lobby.get()
    local peer = l.peers[l.calling_peer_id]
    peer.public_data:set("is_moving", false)
end

function main._can_create_lobby()
    local l = lobby.get()
    if l.max_players ~= 1000 then
        error("max players needs to be 1000")
    end
end

function main._dir_code_to_vector(dir_code)
    if dir_code == player.PLAYER_DIR.DIR_LEFT then
        return Vector2i(-1, 0)
    elseif dir_code == player.PLAYER_DIR.DIR_UP then
        return Vector2i(0, -1)
    elseif dir_code == player.PLAYER_DIR.DIR_RIGHT then
        return Vector2i(1, 0)
    elseif dir_code == player.PLAYER_DIR.DIR_DOWN then
        return Vector2i(0, 1)
    else
        return Vector2i(0, 0)
    end
end

function main._check_peer_collision(l, pos)
    for _, peer_id in ipairs(l.peers:get_keys()) do
        local checking_peer = l.peers[peer_id]
        local checking_pos = Vector2i(checking_peer.public_data:get_int("pos_x"), checking_peer.public_data:get_int("pos_y"))
        if checking_pos == pos then
            return true
        end
    end
    return false
end

function main._move_peer(moving_peer, l, dir_code, move_start)
    local pos = Vector2i(moving_peer.public_data:get_int("pos_x"), moving_peer.public_data:get_int("pos_y"))
    local dir = main._dir_code_to_vector(dir_code)
    local current_id = l.private_data:get_int(pos:to_string())
    local new_id = l.private_data:get_int((pos + dir):to_string())

    moving_peer.public_data:set("is_interactable", map._is_interactable(new_id))
    moving_peer.public_data:set("dir", dir_code)

    if not map._is_walkable(new_id) or main._check_peer_collision(l, pos + dir) then
        moving_peer.public_data:set("is_moving", false)
        return false
    end

    local speed_ms = map._tile_speed(current_id) + map._tile_speed(new_id)
    moving_peer.public_data:set("move_time", speed_ms)
    moving_peer.public_data:set("pos_x", (pos + dir).x)
    moving_peer.public_data:set("pos_y", (pos + dir).y)
    moving_peer.public_data:set("move_start", move_start)
    moving_peer.public_data:set("is_moving", true)
    return true
end

function main._on_lobby_tick(tickrate)
    local l = lobby.get()
    local current_time_ms = get_ticks_ms()
    for _, peer_id in ipairs(l.peers:get_keys()) do
        local peer = l.peers[peer_id]
        if peer.public_data:get_bool("is_moving") then
            local move_start_ms = peer.public_data:get_int("move_start")
            local move_time_ms = peer.public_data:get_int("move_time")
            if move_start_ms + move_time_ms < current_time_ms + tickrate then
                main._move_peer(peer, l, peer.public_data:get_int("dir"), move_start_ms + move_time_ms)
            end
        end
    end
end
