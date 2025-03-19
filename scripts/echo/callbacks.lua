local callbacks = {}
function callbacks.on_create(peerID, minPlayers, maxPlayers)
    return nil
end

function callbacks.on_left(peerID)
end

function callbacks.on_tags(tags)
end
function callbacks.on_ready(peerID, ready)
end

return callbacks
