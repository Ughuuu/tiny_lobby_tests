const WebSocket = require('ws');
const { expect } = require('chai');

function randomGameID() {
    return "00000000-0000-0000-0000-000000000000";
}

function connectWebSocket(gameID) {
    return new Promise((resolve, reject) => {
        const ws = new WebSocket('ws://localhost:8080/connect', ['appsinacup', gameID]);
        ws.on('open', () => {
            ws.once('message', (data) => {
                let response;
                try {
                    response = JSON.parse(data);
                } catch (e) {
                    console.error('Failed to parse initial message:', data.toString(), e);
                    reject(e);
                    return;
                }
                let found = false;
                if (Array.isArray(response)) {
                    for (const msg of response) {
                        if (msg && msg.command === 'peer_state') {
                            found = true;
                            break;
                        }
                    }
                } else if (response && response.command === 'peer_state') {
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

function connectWebSocketWithReconnectToken(gameID, reconnectToken) {
    return new Promise((resolve, reject) => {
        const ws = new WebSocket('ws://localhost:8080/connect', ['appsinacup', gameID, reconnectToken]);
        ws.on('open', () => {
            ws.once('message', (data) => {
                let response;
                try {
                    response = JSON.parse(data);
                } catch (e) {
                    console.error('Failed to parse initial message:', data.toString(), e);
                    reject(e);
                    return;
                }
                let peerMsg = null;
                if (Array.isArray(response)) {
                    for (const msg of response) {
                        if (msg && msg.command === 'peer_state') {
                            peerMsg = msg;
                            break;
                        }
                    }
                } else if (response && response.command === 'peer_state') {
                    peerMsg = response;
                }
                if (!peerMsg) {
                    console.error('Expected peer_state, got:', response);
                }
                expect(peerMsg).to.not.equal(null);
                const peer = peerMsg.data.peer;
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

function sendAndReceive(ws, sendObj, expectedCommand) {
    return new Promise((resolve, reject) => {
        ws.send(JSON.stringify(sendObj), (err) => {
            if (err) return reject(err);
            ws.once('message', (data) => {
                let response;
                try {
                    response = JSON.parse(data);
                } catch (e) {
                    console.error('Failed to parse message:', data.toString(), e);
                    reject(e);
                    return;
                }
                let foundMsg = null;
                if (Array.isArray(response)) {
                    for (const msg of response) {
                        if (msg && msg.command === expectedCommand) {
                            foundMsg = msg;
                            break;
                        }
                    }
                } else if (response && response.command === expectedCommand) {
                    foundMsg = response;
                }
                if (!foundMsg) {
                    console.error(`Expected ${expectedCommand}, got:`, response);
                }
                expect(foundMsg).to.not.equal(null);
                resolve(foundMsg);
            });
        });
    });
}

function createLobby(ws) {
    return sendAndReceive(ws, { command: "create_lobby", data: { max_players: 2, name: "testLobby" } }, "lobby_created")
        .then(response => response.data.lobby.id);
}

function joinLobby(ws, lobbyID, expectedCommand) {
    return sendAndReceive(ws, { command: "join_lobby", data: { lobby_id: lobbyID } }, expectedCommand)
        .then(response => {
            if (expectedCommand !== "error") {
                expect(response.data.lobby.id).to.equal(lobbyID);
            }
        });
}

function readMessage(ws, expectedCommand) {
    return new Promise((resolve, reject) => {
        ws.once('message', (data) => {
            let response;
            try {
                response = JSON.parse(data);
            } catch (e) {
                console.error('Failed to parse message:', data.toString(), e);
                reject(e);
                return;
            }
            let foundMsg = null;
            if (Array.isArray(response)) {
                for (const msg of response) {
                    if (msg && msg.command === expectedCommand) {
                        foundMsg = msg;
                        break;
                    }
                }
            } else if (response && response.command === expectedCommand) {
                foundMsg = response;
            }
            if (!foundMsg) {
                console.error(`Expected ${expectedCommand}, got:`, response);
            }
            expect(foundMsg).to.not.equal(null);
            resolve(foundMsg);
        });
    });
}

describe('Lobby Server', function() {
    this.timeout(10000);

    it('TestCreateLobbySuccess', async () => {
        const ws = await connectWebSocket(randomGameID());
        const lobbyID = await createLobby(ws);
        expect(lobbyID).to.be.a('string').and.not.empty;
        ws.close();
    });

    it('TestCreateLobbyTwiceErrors', async () => {
        const ws = await connectWebSocket(randomGameID());
        await createLobby(ws);
        ws.send(JSON.stringify({ command: "create_lobby", data: { max_players: 2 } }));
        const response = await readMessage(ws, "error");
        expect(response.message).to.exist;
        ws.close();
    });

    it('TestJoinLobbySuccess', async () => {
        const gameID = randomGameID();
        const ws1 = await connectWebSocket(gameID);
        const lobbyID = await createLobby(ws1);
        const ws2 = await connectWebSocket(gameID);
        await joinLobby(ws2, lobbyID, "joined_lobby");

        const response = await readMessage(ws1, "peer_joined");
        expect(response.data).to.exist;
        ws1.close();
        ws2.close();
    });

    it('TestJoinReconnectLobbySuccess', async () => {
        const gameID = randomGameID();
        const ws1 = await connectWebSocket(gameID);
        const lobbyID = await createLobby(ws1);

        let { ws: ws2, reconnectTokenResult: reconnectToken } = await connectWebSocketWithReconnectToken(gameID, "");
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
        const ws = await connectWebSocket(randomGameID());
        const lobbyID = await createLobby(ws);
        await joinLobby(ws, lobbyID, "error");
        ws.close();
    });

    it('TestJoinLobbyInvalidNameErrors', async () => {
        const ws = await connectWebSocket(randomGameID());
        ws.send(JSON.stringify({ command: "join_lobby", data: null }));
        const response = await readMessage(ws, "error");
        expect(response.message).to.exist;
        ws.close();
    });

    it('TestLeaveLobbyCreatedSuccess', async () => {
        const ws = await connectWebSocket(randomGameID());
        await createLobby(ws);
        ws.send(JSON.stringify({ command: "leave_lobby" }));
        await readMessage(ws, "lobby_left");
        ws.close();
    });

    it('TestLeaveLobbyErrors', async () => {
        const ws = await connectWebSocket(randomGameID());
        ws.send(JSON.stringify({ command: "leave_lobby" }));
        const response = await readMessage(ws, "error");
        expect(response.message).to.exist;
        ws.close();
    });

    it('TestLeaveHostLobbySuccess', async () => {
        const gameID = randomGameID();
        const ws1 = await connectWebSocket(gameID);
        const lobbyID = await createLobby(ws1);

        const ws2 = await connectWebSocket(gameID);
        await joinLobby(ws2, lobbyID, "joined_lobby");
        await readMessage(ws1, "peer_joined");

        ws1.send(JSON.stringify({ command: "leave_lobby" }));
        await readMessage(ws1, "lobby_left");
        await readMessage(ws2, "lobby_kicked");
        ws1.close();
        ws2.close();
    });

    it('TestLeavePeerLobbySuccess', async () => {
        const gameID = randomGameID();
        const ws1 = await connectWebSocket(gameID);
        const lobbyID = await createLobby(ws1);

        const ws2 = await connectWebSocket(gameID);
        await joinLobby(ws2, lobbyID, "joined_lobby");
        await readMessage(ws1, "peer_joined");

        ws2.send(JSON.stringify({ command: "leave_lobby" }));
        await readMessage(ws1, "peer_left");
        await readMessage(ws2, "lobby_left");
        ws1.close();
        ws2.close();
    });

    it('TestListLobbies', async () => {
        const ws = await connectWebSocket(randomGameID());
        ws.send(JSON.stringify({ command: "list_lobby" }));
        const response = await readMessage(ws, "lobby_list");
        expect(response.data.lobbies.length).to.be.greaterThan(0);
        ws.close();
    });

    it('TestListLobbiesEmpty', async () => {
        const ws = await connectWebSocket(randomGameID());
        ws.send(JSON.stringify({ command: "list_lobby" }));
        const response = await readMessage(ws, "lobby_list");
        expect(response.data.lobbies.length).to.be.greaterThan(0);
        ws.close();
    });

    it('TestLobbyKickInvalidLobbyErrors', async () => {
        const ws = await connectWebSocket(randomGameID());
        ws.send(JSON.stringify({ command: "kick_peer", data: null }));
        const response = await readMessage(ws, "error");
        expect(response.message).to.exist;
        ws.close();
    });

    it('TestLobbyKickNotHostErrors', async () => {
        const gameID = randomGameID();
        const ws1 = await connectWebSocket(gameID);
        const lobbyID = await createLobby(ws1);

        const ws2 = await connectWebSocket(gameID);
        await joinLobby(ws2, lobbyID, "joined_lobby");
        const response = await readMessage(ws1, "peer_joined");
        const peerId = response.data.peer.id;
        ws2.send(JSON.stringify({ command: "kick_peer", data: { peer: peerId } }));
        const response2 = await readMessage(ws2, "error");
        expect(response2.message).to.exist;
        ws1.close();
        ws2.close();
    });

    it('TestLobbyKickByHostPeerDoesNotExistErrors', async () => {
        const gameID = randomGameID();
        const ws1 = await connectWebSocket(gameID);
        ws1.send(JSON.stringify({ command: "kick_peer", data: { peer: "123" } }));
        const response = await readMessage(ws1, "error");
        expect(response.message).to.exist;
        ws1.close();
    });

    it('TestLobbyKickByHostSuccess', async () => {
        const gameID = randomGameID();
        const ws1 = await connectWebSocket(gameID);
        const lobbyID = await createLobby(ws1);

        const ws2 = await connectWebSocket(gameID);
        await joinLobby(ws2, lobbyID, "joined_lobby");
        const response = await readMessage(ws1, "peer_joined");
        const peerId = response.data.peer.id;
        ws1.send(JSON.stringify({ command: "kick_peer", data: { peer: peerId } }));
        await readMessage(ws1, "peer_left");
        await readMessage(ws2, "lobby_kicked");
        ws1.close();
        ws2.close();
    });

    it('TestPeerReadySuccess', async () => {
        const ws = await connectWebSocket(randomGameID());
        await createLobby(ws);
        ws.send(JSON.stringify({ command: "lobby_ready" }));
        const response = await readMessage(ws, "peer_ready");
        expect(response.data).to.exist;
        ws.close();
    });

    it('TestPeerReadyWithoutLobbyErrors', async () => {
        const ws = await connectWebSocket(randomGameID());
        ws.send(JSON.stringify({ command: "lobby_ready" }));
        const response = await readMessage(ws, "error");
        expect(response.message).to.exist;
        ws.close();
    });

    it('TestLobbySealSuccess', async () => {
        const ws = await connectWebSocket(randomGameID());
        await createLobby(ws);
        ws.send(JSON.stringify({ command: "seal_lobby" }));
        await readMessage(ws, "lobby_sealed");
        ws.close();
    });

    it('TestLobbySealWithoutLobbyErrors', async () => {
        const ws = await connectWebSocket(randomGameID());
        ws.send(JSON.stringify({ command: "seal_lobby" }));
        await readMessage(ws, "error");
        ws.close();
    });

    it('TestLobbySealNotHostErrors', async () => {
        const gameID = randomGameID();
        const ws1 = await connectWebSocket(gameID);
        const lobbyID = await createLobby(ws1);

        const ws2 = await connectWebSocket(gameID);
        await joinLobby(ws2, lobbyID, "joined_lobby");
        const response = await readMessage(ws1, "peer_joined");
        ws2.send(JSON.stringify({ command: "seal_lobby", data: response.data }));
        const response2 = await readMessage(ws2, "error");
        expect(response2.message).to.exist;
        ws1.close();
        ws2.close();
    });

    it('TestLobbyUnsealSuccess', async () => {
        const ws = await connectWebSocket(randomGameID());
        await createLobby(ws);
        ws.send(JSON.stringify({ command: "seal_lobby" }));
        await readMessage(ws, "lobby_sealed");
        ws.send(JSON.stringify({ command: "unseal_lobby" }));
        await readMessage(ws, "lobby_unsealed");
        ws.close();
    });

    it('TestLobbyUnsealWithoutLobbyErrors', async () => {
        const ws = await connectWebSocket(randomGameID());
        ws.send(JSON.stringify({ command: "unseal_lobby" }));
        await readMessage(ws, "error");
        ws.close();
    });

    it('TestLobbyUnsealNotHostErrors', async () => {
        const gameID = randomGameID();
        const ws1 = await connectWebSocket(gameID);
        const lobbyID = await createLobby(ws1);

        const ws2 = await connectWebSocket(gameID);
        await joinLobby(ws2, lobbyID, "joined_lobby");
        ws2.send(JSON.stringify({ command: "unseal_lobby" }));
        await readMessage(ws2, "error");
        ws1.close();
        ws2.close();
    });

    it('TestLobbyTagsSuccess', async () => {
        const ws = await connectWebSocket(randomGameID());
        await createLobby(ws);
        ws.send(JSON.stringify({ command: "lobby_tags", data: { tags: { tag1: "val1", tag2: 2.0 } } }));
        const tagsResponse = await readMessage(ws, "lobby_tags");
        const tagsData = tagsResponse.data;
        expect(tagsData.tags.tag1).to.equal("val1");
        expect(tagsData.tags.tag2).to.equal(2.0);
        ws.close();
    });
});
