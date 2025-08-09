import { TinyLobbyClient, LobbyEvent } from './tiny-lobby-client';

async function main() {
    const client = new TinyLobbyClient({
        gameId: 'your-game-id',
        debug: true
    });

    // Set up event listeners
    client.on(LobbyEvent.CONNECTED, ({ peer, reconnectionToken }) => {
        console.log("Connected as peer:", peer.id);
        console.log("Reconnection token:", reconnectionToken);
    });

    client.on(LobbyEvent.LOBBY_JOINED, ({ lobby, peers }) => {
        console.log(`Joined lobby: ${lobby.name} (${lobby.id})`);
        console.log(`Peers in lobby: ${peers.length}`);
    });

    client.on(LobbyEvent.PEER_JOINED, ({ peer }) => {
        console.log(`Peer joined: ${peer.id}`);
    });

    client.on(LobbyEvent.CHAT_MESSAGE, (message) => {
        console.log(`[CHAT] ${message.fromPeerId}: ${message.content}`);
    });

    client.on(LobbyEvent.ERROR, (error) => {
        console.error(`Error: ${error.code} - ${error.message}`);
    });

    try {
        // Connect to the server
        await client.connect();

        // Join a lobby
        const lobby = await client.quickJoin("My Awesome Lobby");

        // Send a chat message
        await client.sendChatMessage("Hello everyone!");

        // Set user data
        await client.setUserData({
            avatar: "warrior",
            level: 5,
            ready: true
        });

        // Set ready status
        await client.setReady(true);

        // Call a lobby function
        const result = await client.callLobbyFunction("startGame", []);
        console.log("Game started with result:", result);

    } catch (error) {
        console.error("Failed:", error);
    } finally {
        client.disconnect();
    }
}

main();
