local helper = {}

function helper.peers_ordered(l)
    local peerIDs = {}

    -- Collect all peer IDs
    for peerID, _ in pairs(l.Peers) do
        table.insert(peerIDs, peerID)
    end

    table.sort(peerIDs, function(a, b)
        return l.Peers[a].OrderID < l.Peers[b].OrderID
    end)

    return peerIDs
end

function helper.map_length(dict)
    local length = 0
    for _, _ in pairs(dict) do
        length = length + 1
    end
    return length
end

return helper
