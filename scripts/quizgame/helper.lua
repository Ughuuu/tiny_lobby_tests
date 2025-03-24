local helper = {}

function helper.peers_ordered(l)
    local peerIDs = {}
    -- Collect all peer IDs
    for peerID, _ in pairs(l.peers) do
        table.insert(peerIDs, peerID)
    end

    table.sort(peerIDs, function(a, b)
        return l.peers[a].order_id < l.peers[b].order_id
    end)

    return peerIDs
end

return helper