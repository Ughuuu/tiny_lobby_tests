local lobby = require("lobby")

function echo(message)
    local l = lobby.get()
    local a = l.id
    local b = l.name
    local c = l.host
    local d = l.max_players
    local e = l.create_time
    local f = l.sealed
    local g = l.tags
    local h = l.public_data
    local i = l.private_data
    local d = l.peers
    -- print(lobby.id)
    return message
end

-- Callback functions
local callbacks = require("callbacks")
_on_create = function() return callbacks.on_create(2, 10) end
_on_join = callbacks.on_join
_on_chat = callbacks.on_chat
_on_tags = callbacks.on_tags
_on_kick = callbacks.on_kick
_on_ready = callbacks.on_ready
_on_seal = callbacks.on_seal
_on_left = callbacks.on_left
