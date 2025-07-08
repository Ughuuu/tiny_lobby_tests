local main = {}
-- Load Modules
local api = require("api")
local lobby = require("lobby")

-- Callable Function
main.start_game = api.start_game
main.move_piece = function(fromX, fromY, toX, toY, promotionPiece) return api.move_piece(fromX, fromY, toX, toY, promotionPiece) end
main.me_command = function (action)
    if type(action) ~= "string" then
        return { error = "Invalid action format." }
    end
    local l = lobby.get()
    local peer_name = l.peers[l.calling_peer_id].user_data["name"]
    l.broadcast_chat(string.format("* %s %s", peer_name, action))
    return
end
main.ripple = function (pos_x, pos_y)
    lobby.get().notify_all({
        type = "ripple",
        pos_x = pos_x,
        pos_y = pos_y
    })
end

-- Callback functions
local callbacks = require("callbacks")
main._can_create_lobby = function() return callbacks.can_create(2, 2) end
main._on_lobby_created = function() return callbacks.on_create() end
main._on_peer_leave = callbacks.on_left
main._can_host_set_tags = callbacks.on_tags
main._can_peer_ready = callbacks.on_ready

return main
