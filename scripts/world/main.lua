local lobby = require("lobby")
local system = require("system")
local main = {}

local MOVE_DELAY_SEC = 250
local INPUT_DELAY_SEC = 50
local DIRECTIONS = {
  up =    { x =  0, y = -1 },
  down =  { x =  0, y =  1 },
  left =  { x = -1, y =  0 },
  right = { x =  1, y =  0 }
}

function main._on_lobby_created()
    local l = lobby.get()
    local peer = l.peers[l.calling_peer_id]
    main._set_peer_initial_data(peer)
end

function main._on_peer_joined()
    local l = lobby.get()
    local peer = l.peers[l.calling_peer_id]
    main._set_peer_initial_data(peer)
    return
end

function main._set_peer_initial_data(peer)
    peer.public_data["pos"] = { x = 0, y = 0 }
    peer.public_data["dir"] = { x = 1, y = 0 }
    peer.public_data["move_start"] = 0
    peer.public_data["is_moving"] = false
end

function main.move(dir: string)
    local l = lobby.get()
    local peer = l.peers[l.calling_peer_id]
    if dir ~= "up" and dir ~= "down" and dir ~= "left" and dir ~= "right" then
        return { error = "invalid dir" }
    end

    local move_start_ms = peer.public_data["move_start"]
    local current_time_ms = system.get_time_since_epoch()
    if move_start_ms + MOVE_DELAY_SEC - INPUT_DELAY_SEC > current_time_ms then
        return { error = "too soon" }
    end

    main._move_peer(peer, l, dir, current_time_ms + MOVE_DELAY_SEC - INPUT_DELAY_SEC)
    return
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
    local pos = moving_peer.public_data["pos"]
    local dir = DIRECTIONS[dir_code]

    moving_peer.public_data["dir"] = dir
    moving_peer.public_data["pos"] = { x = pos["x"] + dir["x"], y = pos["y"] + dir["y"] }
    moving_peer.public_data["move_start"] = move_start
end

return main
