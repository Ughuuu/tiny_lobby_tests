const WebSocket = require('ws');
const { expect } = require('chai');

const msg_error = 0 
const msg_lobby_hosted = 1 
const msg_lobby_created = 2 
const msg_lobby_unsealed = 3 
const msg_lobby_sealed = 4 
const msg_lobby_resized = 5 
const msg_lobby_passworded = 6 
const msg_lobby_titled = 7 
const msg_peer_ready = 8 
const msg_peer_unready = 9 
const msg_lobby_left = 10 
const msg_peer_left = 11 
const msg_lobby_kicked = 12 
const msg_peer_state = 13 
const msg_lobby_call = 14 
const msg_peer_user_d = 15 
const msg_lobby_tags = 16 
const msg_peer_chat = 17 
const msg_peer_reconnected = 18 
const msg_joined_lobby = 19 
const msg_peer_joined = 20 
const msg_peer_disconnected = 21 
const msg_lobby_list = 22 
const msg_lobby_d = 23 
const msg_peer_notify = 24 
const msg_d_to = 25 
const msg_d_to_sent = 26 
const msg_notify_to_sent = 27 

function relayGameID() {
    return "00000000-0000-0000-0000-000000000000";
}
// Helper: buffer all messages for a WebSocket
function bufferWebSocket(ws) {
    ws._buffer = [];
    ws.on('message', (d) => {
        ws._buffer.push(d);
    });
}
function connectWebSocket(gameID) {
    return new Promise((resolve, reject) => {
        const ws = new WebSocket('ws://localhost:8080/connect', ['appsinacup', gameID]);
        bufferWebSocket(ws);
        ws.on('open', () => {
            ws.once('message', (d) => {
                let response;
                try {
                    response = JSON.parse(d);
                } catch (e) {
                    console.error('Failed to parse initial message:', d.toString(), e);
                    reject(e);
                    return;
                }
                let found = false;
                if (Array.isArray(response)) {
                    for (const msg of response) {
                        if (msg && msg.c === msg_peer_state) {
                            found = true;
                            break;
                        }
                    }
                } else if (response && response.c === msg_peer_state) {
                    found = true;
                }
                if (!found) {
                    console.error('Expected peer_state, got:', response);
                }
                expect(found).to.equal(true);
                resolve(ws);
            });
        });
        ws.on('error', (err) => {
            console.error('WebSocket error:', err);
            reject(err);
        });
    });
}

function connectWebSocketAndGetReconnectToken(gameID) {
    return new Promise((resolve, reject) => {
        const ws = new WebSocket('ws://localhost:8080/connect', ['appsinacup', gameID]);
        bufferWebSocket(ws);
        ws.on('open', () => {
            ws.once('message', (d) => {
                let response;
                try {
                    response = JSON.parse(d);
                } catch (e) {
                    console.error('Failed to parse initial message:', d.toString(), e);
                    reject(e);
                    return;
                }
                let peerMsg = null;
                if (Array.isArray(response)) {
                    for (const msg of response) {
                        if (msg && msg.c === msg_peer_state) {
                            peerMsg = msg;
                            break;
                        }
                    }
                } else if (response && response.c === msg_peer_state) {
                    peerMsg = response;
                }
                if (!peerMsg) {
                    console.error('Expected peer_state, got:', response);
                }
                expect(peerMsg).to.not.equal(null);
                const peer = peerMsg.d.peer;
                const reconnectToken = peer.reconnection_token;
                if (!reconnectToken) {
                    console.error('Expected reconnection_token in peer state, got:', peer);
                    reject(new Error('No reconnection token found'));
                    return;
                }
                resolve({ ws, reconnectToken });
            });
        });
        ws.on('error', (err) => {
            console.error('WebSocket error:', err);
            reject(err);
        });
    });
}

function connectWebSocketWithReconnectToken(gameID, reconnectToken) {
    return new Promise((resolve, reject) => {
        const ws = new WebSocket('ws://localhost:8080/connect', ['appsinacup', gameID, reconnectToken]);
        bufferWebSocket(ws);
        ws.on('open', () => {
            ws.once('message', (d) => {
                let response;
                try {
                    response = JSON.parse(d);
                } catch (e) {
                    console.error('Failed to parse initial message:', d.toString(), e);
                    reject(e);
                    return;
                }
                let peerMsg = null;
                if (Array.isArray(response)) {
                    for (const msg of response) {
                        if (msg && msg.c === msg_peer_state) {
                            peerMsg = msg;
                            break;
                        }
                    }
                } else if (response && response.c === msg_peer_state) {
                    peerMsg = response;
                }
                if (!peerMsg) {
                    console.error('Expected peer_state, got:', response);
                }
                expect(peerMsg).to.not.equal(null);
                const peer = peerMsg.d.peer;
                const reconnectTokenResult = peer.reconnection_token;
                resolve({ ws, reconnectTokenResult });
            });
        });
        ws.on('error', (err) => {
            console.error('WebSocket error:', err);
            reject(err);
        });
    });
}

function sendAndReceive(ws, sendObj, expectedc) {
    return new Promise((resolve, reject) => {
        ws.send(JSON.stringify(sendObj), (err) => {
            if (err) return reject(err);
            ws.once('message', (d) => {
                let response;
                try {
                    response = JSON.parse(d);
                } catch (e) {
                    console.error('Failed to parse message:', d.toString(), e);
                    reject(e);
                    return;
                }
                let foundMsg = null;
                if (Array.isArray(response)) {
                    for (const msg of response) {
                        if (msg && msg.c === expectedc) {
                            foundMsg = msg;
                            break;
                        }
                    }
                } else if (response && response.c === expectedc) {
                    foundMsg = response;
                }
                if (!foundMsg) {
                    console.error(`Expected ${expectedc}, got:`, response);
                }
                expect(foundMsg).to.not.equal(null);
                resolve(foundMsg);
            });
        });
    });
}

function createLobby(ws) {
    return sendAndReceive(ws, { c: "create_lobby", d: { m: 2, name: "testLobby" } }, msg_lobby_created)
        .then(response => response.d.lobby.id);
}

function joinLobby(ws, lobbyID, expectedc) {
    return sendAndReceive(ws, { c: "join_lobby", d: { lobby_id: lobbyID } }, expectedc)
        .then(response => {
            if (expectedc !== msg_error) {
                expect(response.d.lobby.id).to.equal(lobbyID);
            }
        });
}

function readMessage(ws, expectedc) {
    return new Promise((resolve, reject) => {
        // Helper to extract and return the expected message from a buffer
        function extractFromBuffer() {
            if (ws._buffer && ws._buffer.length > 0) {
                // Search for a message with the expected code
                for (let i = 0; i < ws._buffer.length; ++i) {
                    let d = ws._buffer[i];
                    let response;
                    try {
                        response = JSON.parse(d);
                    } catch (e) {
                        continue;
                    }
                    let foundMsg = null;
                    if (Array.isArray(response)) {
                        for (const msg of response) {
                            if (msg && msg.c === expectedc) {
                                foundMsg = msg;
                                break;
                            }
                        }
                    } else if (response && response.c === expectedc) {
                        foundMsg = response;
                    }
                    if (foundMsg) {
                        // Remove this message from the buffer
                        ws._buffer.splice(i, 1);
                        resolve(foundMsg);
                        return true;
                    }
                }
            }
            return false;
        }

        // Try to extract immediately
        if (extractFromBuffer()) return;

        // Otherwise, wait for new messages
        function onMessage(d) {
            ws._buffer = ws._buffer || [];
            ws._buffer.push(d);
            if (extractFromBuffer()) {
                ws.removeListener('message', onMessage);
            }
        }
        ws.on('message', onMessage);
    });
}

describe('Lobby Server', function() {
    this.timeout(10000);

    it('TestConnectSuccess', async () => {
        const ws = await connectWebSocket(relayGameID());
        expect(ws).to.exist;
        ws.close();
    });

    it('TestCreateLobbySuccess', async () => {
        const ws = await connectWebSocket(relayGameID());
        const lobbyID = await createLobby(ws);
        expect(lobbyID).to.be.a('string').and.not.empty;
        ws.close();
    });

    it('TestCreateLobbyTwiceErrors', async () => {
        const ws = await connectWebSocket(relayGameID());
        await createLobby(ws);
        ws.send(JSON.stringify({ c: "create_lobby", d: { m: 2 } }));
        const response = await readMessage(ws, msg_error);
        expect(response.m).to.exist;
        ws.close();
    });

    it('TestJoinLobbySuccess', async () => {
        const gameID = relayGameID();
        const ws1 = await connectWebSocket(gameID);
        ws1.on('message', (d) => {
            console.log('[WS RECV]', d.toString());
        });
        const lobbyID = await createLobby(ws1);
        const ws2 = await connectWebSocket(gameID);
        await joinLobby(ws2, lobbyID, msg_joined_lobby);

        const response = await readMessage(ws1, msg_peer_joined);
        expect(response.d).to.exist;
        ws1.close();
        ws2.close();
    });

    it.only('TestJoinReconnectLobbySuccess', async () => {
        const gameID = relayGameID();
        const ws1 = await connectWebSocket(gameID);
        const lobbyID = await createLobby(ws1);

        let { ws: ws2, reconnectTokenResult: reconnectToken } = await connectWebSocketAndGetReconnectToken(gameID);
        await joinLobby(ws2, lobbyID, "joined_lobby");
        await readMessage(ws1, "peer_joined");

        ws2.close();
        await readMessage(ws1, "peer_disconnected");

        let { ws: ws22, reconnectTokenResult: reconnectToken2 } = await connectWebSocketWithReconnectToken(gameID, reconnectToken);
        expect(reconnectToken2).to.not.equal(reconnectToken);
        await joinLobby(ws22, lobbyID, "joined_lobby");
        await readMessage(ws1, "peer_reconnected");
        ws1.close();
        ws22.close();
    });

    it('TestJoinLobbySamePeerErrors', async () => {
        const ws = await connectWebSocket(relayGameID());
        const lobbyID = await createLobby(ws);
        await joinLobby(ws, lobbyID, "error");
        ws.close();
    });

    it('TestJoinLobbyInvalidNameErrors', async () => {
        const ws = await connectWebSocket(relayGameID());
        ws.send(JSON.stringify({ c: "join_lobby", d: null }));
        const response = await readMessage(ws, "error");
        expect(response.m).to.exist;
        ws.close();
    });

    it('TestLeaveLobbyCreatedSuccess', async () => {
        const ws = await connectWebSocket(relayGameID());
        await createLobby(ws);
        ws.send(JSON.stringify({ c: "leave_lobby" }));
        await readMessage(ws, "lobby_left");
        ws.close();
    });

    it('TestLeaveLobbyErrors', async () => {
        const ws = await connectWebSocket(relayGameID());
        ws.send(JSON.stringify({ c: "leave_lobby" }));
        const response = await readMessage(ws, "error");
        expect(response.m).to.exist;
        ws.close();
    });

    it('TestLeaveHostLobbySuccess', async () => {
        const gameID = relayGameID();
        const ws1 = await connectWebSocket(gameID);
        const lobbyID = await createLobby(ws1);

        const ws2 = await connectWebSocket(gameID);
        await joinLobby(ws2, lobbyID, "joined_lobby");
        await readMessage(ws1, "peer_joined");

        ws1.send(JSON.stringify({ c: "leave_lobby" }));
        await readMessage(ws1, "lobby_left");
        await readMessage(ws2, "lobby_kicked");
        ws1.close();
        ws2.close();
    });

    it('TestLeavePeerLobbySuccess', async () => {
        const gameID = relayGameID();
        const ws1 = await connectWebSocket(gameID);
        const lobbyID = await createLobby(ws1);

        const ws2 = await connectWebSocket(gameID);
        await joinLobby(ws2, lobbyID, "joined_lobby");
        await readMessage(ws1, "peer_joined");

        ws2.send(JSON.stringify({ c: "leave_lobby" }));
        await readMessage(ws1, "peer_left");
        await readMessage(ws2, "lobby_left");
        ws1.close();
        ws2.close();
    });

    it('TestListLobbies', async () => {
        const ws = await connectWebSocket(relayGameID());
        ws.send(JSON.stringify({ c: "list_lobby" }));
        const response = await readMessage(ws, "lobby_list");
        expect(response.d.lobbies.length).to.be.greaterThan(0);
        ws.close();
    });

    it('TestListLobbiesEmpty', async () => {
        const ws = await connectWebSocket(relayGameID());
        ws.send(JSON.stringify({ c: "list_lobby" }));
        const response = await readMessage(ws, "lobby_list");
        expect(response.d.lobbies.length).to.be.greaterThan(0);
        ws.close();
    });

    it('TestLobbyKickInvalidLobbyErrors', async () => {
        const ws = await connectWebSocket(relayGameID());
        ws.send(JSON.stringify({ c: "kick_peer", d: null }));
        const response = await readMessage(ws, "error");
        expect(response.m).to.exist;
        ws.close();
    });

    it('TestLobbyKickNotHostErrors', async () => {
        const gameID = relayGameID();
        const ws1 = await connectWebSocket(gameID);
        const lobbyID = await createLobby(ws1);

        const ws2 = await connectWebSocket(gameID);
        await joinLobby(ws2, lobbyID, "joined_lobby");
        const response = await readMessage(ws1, "peer_joined");
        const peerId = response.d.peer.id;
        ws2.send(JSON.stringify({ c: "kick_peer", d: { peer: peerId } }));
        const response2 = await readMessage(ws2, "error");
        expect(response2.m).to.exist;
        ws1.close();
        ws2.close();
    });

    it('TestLobbyKickByHostPeerDoesNotExistErrors', async () => {
        const gameID = relayGameID();
        const ws1 = await connectWebSocket(gameID);
        ws1.send(JSON.stringify({ c: "kick_peer", d: { peer: "123" } }));
        const response = await readMessage(ws1, "error");
        expect(response.m).to.exist;
        ws1.close();
    });

    it('TestLobbyKickByHostSuccess', async () => {
        const gameID = relayGameID();
        const ws1 = await connectWebSocket(gameID);
        const lobbyID = await createLobby(ws1);

        const ws2 = await connectWebSocket(gameID);
        await joinLobby(ws2, lobbyID, "joined_lobby");
        const response = await readMessage(ws1, "peer_joined");
        const peerId = response.d.peer.id;
        ws1.send(JSON.stringify({ c: "kick_peer", d: { peer: peerId } }));
        await readMessage(ws1, "peer_left");
        await readMessage(ws2, "lobby_kicked");
        ws1.close();
        ws2.close();
    });

    it('TestPeerReadySuccess', async () => {
        const ws = await connectWebSocket(relayGameID());
        await createLobby(ws);
        ws.send(JSON.stringify({ c: "lobby_ready" }));
        const response = await readMessage(ws, "peer_ready");
        expect(response.d).to.exist;
        ws.close();
    });

    it('TestPeerReadyWithoutLobbyErrors', async () => {
        const ws = await connectWebSocket(relayGameID());
        ws.send(JSON.stringify({ c: "lobby_ready" }));
        const response = await readMessage(ws, "error");
        expect(response.m).to.exist;
        ws.close();
    });

    it('TestLobbySealSuccess', async () => {
        const ws = await connectWebSocket(relayGameID());
        await createLobby(ws);
        ws.send(JSON.stringify({ c: "seal_lobby" }));
        await readMessage(ws, "lobby_sealed");
        ws.close();
    });

    it('TestLobbySealWithoutLobbyErrors', async () => {
        const ws = await connectWebSocket(relayGameID());
        ws.send(JSON.stringify({ c: "seal_lobby" }));
        await readMessage(ws, "error");
        ws.close();
    });

    it('TestLobbySealNotHostErrors', async () => {
        const gameID = relayGameID();
        const ws1 = await connectWebSocket(gameID);
        const lobbyID = await createLobby(ws1);

        const ws2 = await connectWebSocket(gameID);
        await joinLobby(ws2, lobbyID, "joined_lobby");
        const response = await readMessage(ws1, "peer_joined");
        ws2.send(JSON.stringify({ c: "seal_lobby", d: response.d }));
        const response2 = await readMessage(ws2, "error");
        expect(response2.m).to.exist;
        ws1.close();
        ws2.close();
    });

    it('TestLobbyUnsealSuccess', async () => {
        const ws = await connectWebSocket(relayGameID());
        await createLobby(ws);
        ws.send(JSON.stringify({ c: "seal_lobby" }));
        await readMessage(ws, "lobby_sealed");
        ws.send(JSON.stringify({ c: "unseal_lobby" }));
        await readMessage(ws, "lobby_unsealed");
        ws.close();
    });

    it('TestLobbyUnsealWithoutLobbyErrors', async () => {
        const ws = await connectWebSocket(relayGameID());
        ws.send(JSON.stringify({ c: "unseal_lobby" }));
        await readMessage(ws, "error");
        ws.close();
    });

    it('TestLobbyUnsealNotHostErrors', async () => {
        const gameID = relayGameID();
        const ws1 = await connectWebSocket(gameID);
        const lobbyID = await createLobby(ws1);

        const ws2 = await connectWebSocket(gameID);
        await joinLobby(ws2, lobbyID, "joined_lobby");
        ws2.send(JSON.stringify({ c: "unseal_lobby" }));
        await readMessage(ws2, "error");
        ws1.close();
        ws2.close();
    });

    it('TestLobbyTagsSuccess', async () => {
        const ws = await connectWebSocket(relayGameID());
        await createLobby(ws);
        ws.send(JSON.stringify({ c: "lobby_tags", d: { tags: { tag1: "val1", tag2: 2.0 } } }));
        const tagsResponse = await readMessage(ws, "lobby_tags");
        const tagsd = tagsResponse.d;
        expect(tagsd.tags.tag1).to.equal("val1");
        expect(tagsd.tags.tag2).to.equal(2.0);
        ws.close();
    });
});
