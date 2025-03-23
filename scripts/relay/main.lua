local lobby = require("lobby")

function add_public_data(key, data)
    local l = lobby.get()
    if l.calling_peer_id ~= l.host then
        return { error = "Only host can do this." }
    end
    l.public_data[key] = data
end

function add_peer_public_data(target_peer, key, data)
    local l = lobby.get()
    if l.calling_peer_id ~= l.host then
        return { error = "Only host can do this." }
    end
    l.peers[target_peer].public_data[key] = data
end

function add_peers_public_data(key, data)
    local l = lobby.get()
    if l.calling_peer_id ~= l.host then
        return { error = "Only host can do this." }
    end
    for k, v in pairs(lobby.peers) do
        l.peers[k].public_data[key] = data
    end
end

function add_peer_private_data(target_peer, key, data)
    local l = lobby.get()
    if l.calling_peer_id ~= l.host then
        return { error = "Only host can do this." }
    end
    l.peers[target_peer].private_data[key] = data
end

function add_peers_private_data(key, data)
    local l = lobby.get()
    if l.calling_peer_id ~= l.host then
        return { error = "Only host can do this." }
    end
    for k, v in pairs(l.peers) do
        l.peers[k].private_data[key] = data
    end
end

function notify_peer(target_peer, message)
    local l = lobby.get()
    if l.calling_peer_id ~= l.host then
        return { error = "Only host can do this." }
    end
    lobby.notify(target_peer, message)
end

function notify_peers(message)
    local l = lobby.get()
    if l.calling_peer_id ~= l.host then
        return { error = "Only host can do this." }
    end
    for k, v in pairs(lobby.peers) do
        lobby.notify(k, message)
    end
end

function get_private_data()
    local l = lobby.get()
    if l.calling_peer_id ~= l.host then
        return { error = "Only host can do this." }
    end
    return l.private_data
end
