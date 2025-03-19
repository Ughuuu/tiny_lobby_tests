function echo(peerID, message)
    local a = lobby.id
    local b = lobby.name
    local c = lobby.host
    local d = lobby.max_players
    local e = lobby.create_time
    local f = lobby.sealed
    local g = lobby.tags
    local h = lobby.public_data
    local i = lobby.private_data
    local d = lobby.peers
    -- print(lobby.id)
    return message
end

-- Callback functions
local callbacks = require("callbacks")
_on_create = function(peerID) return callbacks.on_create(peerID, 2, 10) end
_on_join = callbacks.on_join
_on_chat = callbacks.on_chat
_on_tags = callbacks.on_tags
_on_kick = callbacks.on_kick
_on_ready = callbacks.on_ready
_on_seal = callbacks.on_seal
_on_left = callbacks.on_left
