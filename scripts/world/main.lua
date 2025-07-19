local lobby = require("lobby")
local system = require("system")
local main = {}

local MOVE_DELAY_SEC = 250
local INPUT_DELAY_SEC = 150
local DIRECTIONS = {
  up =    { x =  0, y = -1 },
  down =  { x =  0, y =  1 },
  left =  { x = -1, y =  0 },
  right = { x =  1, y =  0 }
}
local map = {}

function main._on_server_init()
    main._load_map()
end
function main._on_server_reload()
    main._load_map()
end

function main._load_map()
    local map_str = system.read_file_as_string("map.json")
    local map_data = system.decode_json(map_str)
    for i = 1, #map_data do
        local cell = map_data[i]
        map[cell.x .. ":" .. cell.y] = cell.t
    end
end

function main._on_lobby_created()
    local l = lobby.get()
    l.host = ""
    l.max_peers = 101
    local peer = l.peers[l.calling_peer_id]
    main._set_peer_initial_data(peer)
end

function main._on_peer_joined()
    local l = lobby.get()
    local peer = l.peers[l.calling_peer_id]
    main._set_peer_initial_data(peer)
    return
end

function main._on_peer_disconnected()
    local l = lobby.get()
    local peer = l.peers[l.calling_peer_id]
    l.kick_peer(peer.id)
    print("Peer disconnected: " .. peer.id)
end

function main._set_peer_initial_data(peer)
    peer.public_data["pos"] = { x = 0, y = 0 }
    peer.public_data["move_start"] = system.get_time_since_epoch()
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
    if move_start_ms + MOVE_DELAY_SEC < current_time_ms then
        move_start_ms = current_time_ms - MOVE_DELAY_SEC
    end

    return main._move_peer(peer, l, dir, move_start_ms + MOVE_DELAY_SEC)
end

function main._move_peer(moving_peer, l, dir_code, move_start)
    local pos = moving_peer.public_data["pos"]
    local dir = DIRECTIONS[dir_code]
    local new_pos = { x = pos["x"] + dir["x"], y = pos["y"] + dir["y"] }
    local current_cell_type = map[pos.x .. ":" .. pos.y]
    local new_cell_type = map[new_pos.x .. ":" .. new_pos.y]
    if not new_cell_type or math.abs(current_cell_type - new_cell_type) > 1 then
        return { error = "invalid move" }
    end
    moving_peer.public_data["pos"] = new_pos
    moving_peer.public_data["move_start"] = move_start
    return
end

return main
