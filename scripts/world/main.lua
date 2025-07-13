local lobby = require("lobby")
local main = {}

local MOVE_DELAY_MS = 250
local DIRECTIONS = {
  up =    { x =  0, y = -1 },
  down =  { x =  0, y =  1 },
  left =  { x = -1, y =  0 },
  right = { x =  1, y =  0 }
}

function main.move(dir: string)
    local l = lobby.get()
    local peer = l.peers[l.calling_peer_id]
    if dir ~= "up" and dir ~= "down" and dir ~= "left" and dir ~= "right" then
        error("invalid dir")
    end

    local move_start_ms = peer.public_data["move_start"]
    local move_time_ms = peer.public_data["move_time"]
    local utcTimestamp = os.time()
    print("UTC Timestamp:", utcTimestamp)
    local current_time_ms = utcTimestamp

    if move_start_ms + move_time_ms > current_time_ms then
        return
    end

    if peer.public_data:get_bool("is_moving") and dir == peer.public_data:get_int("dir") then
        return
    end

    main._move_peer(peer, l, dir, current_time_ms)
end

function main._check_peer_collision(l, pos)
    for _, peer_id in ipairs(l.peers:get_keys()) do
        local checking_peer = l.peers[peer_id]
        local checking_pos = checking_peer.public_data["pos"]
        if checking_pos.x == pos.x and checking_pos.y == pos.y then
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

return main
