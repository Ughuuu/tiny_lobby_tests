const MESSAGE_TYPES = {
    ERROR: 0,
    LOBBY_HOSTED: 1,
    LOBBY_CREATED: 2,
    LOBBY_UNSEALED: 3,
    LOBBY_SEALED: 4,
    LOBBY_RESIZED: 5,
    LOBBY_PASSWORDED: 6,
    LOBBY_TITLED: 7,
    PEER_READY: 8,
    PEER_UNREADY: 9,
    LOBBY_LEFT: 10,
    PEER_LEFT: 11,
    LOBBY_KICKED: 12,
    PEER_STATE: 13,
    LOBBY_CALL: 14,
    PEER_USER_DATA: 15,
    LOBBY_TAGS: 16,
    PEER_CHAT: 17,
    PEER_RECONNECTED: 18,
    JOINED_LOBBY: 19,
    PEER_JOINED: 20,
    PEER_DISCONNECTED: 21,
    LOBBY_LIST: 22,
    LOBBY_DATA: 23,
    PEER_NOTIFY: 24,
    DIRECT_TO: 25,
    DIRECT_TO_SENT: 26,
    NOTIFY_TO_SENT: 27
} as const;

type MessageCode = typeof MESSAGE_TYPES[keyof typeof MESSAGE_TYPES];

interface LobbyMessage {
    c: MessageCode; // message code
    d?: any;       // message data
    m?: string;    // error message (for errors)
}

interface Peer {
    id: string;
    name?: string;
    ready?: boolean;
    reconnection_token?: string;
    user_data?: any;
}

interface Lobby {
    id: string;
    name?: string;
    max_players?: number;
    sealed?: boolean;
    password?: string;
    tags?: Record<string, any>;
    peers?: Peer[];
    host_id?: string;
}

interface LobbyClientOptions {
    gameID?: string;
    reconnectToken?: string;
    autoReconnect?: boolean;
    reconnectInterval?: number;
    maxReconnectAttempts?: number;
}

class LobbyClient {
    private ws: WebSocket | null = null;
    private messageQueue: LobbyMessage[] = [];
    private messageHandlers: Map<MessageCode, ((msg: LobbyMessage) => void)[]> = new Map();
    private connectionPromise: Promise<void> | null = null;
    private reconnectAttempts = 0;
    private options: Required<LobbyClientOptions>;
    private peerState: Peer | null = null;
    private lobbyState: Lobby | null = null;

    // Event emitters (in a real implementation you might want to use EventEmitter)
    public onConnect: (() => void) | null = null;
    public onDisconnect: ((reason: string) => void) | null = null;
    public onError: ((error: Error) => void) | null = null;
    public onLobbyUpdate: ((lobby: Lobby) => void) | null = null;
    public onPeerUpdate: ((peer: Peer) => void) | null = null;

    constructor(private serverUrl: string, options: LobbyClientOptions = {}) {
        this.options = {
            gameID: '00000000-0000-0000-0000-000000000000',
            autoReconnect: true,
            reconnectInterval: 3000,
            maxReconnectAttempts: 5,
            ...options
        };
    }

    public async connect(): Promise<void> {
        if (this.connectionPromise) {
            return this.connectionPromise;
        }

        this.connectionPromise = new Promise(async (resolve, reject) => {
            try {
                const protocols = ['appsinacup', this.options.gameID];
                if (this.options.reconnectToken) {
                    protocols.push(this.options.reconnectToken);
                }

                this.ws = new WebSocket(this.serverUrl, protocols);
                this.setupWebSocketHandlers(resolve, reject);
            } catch (error) {
                this.handleConnectionError(error as Error, reject);
            }
        });

        return this.connectionPromise;
    }

    private setupWebSocketHandlers(resolve: () => void, reject: (reason?: any) => void) {
        if (!this.ws) return;

        this.ws.onopen = () => {
            this.reconnectAttempts = 0;
            resolve();
            this.onConnect?.();
        };

        this.ws.onmessage = (event) => {
            try {
                const message = JSON.parse(event.data.toString());
                this.handleMessage(message);
            } catch (error) {
                this.onError?.(new Error(`Failed to parse message: ${error}`));
            }
        };

        this.ws.onerror = (event) => {
            const error = new Error('WebSocket error occurred');
            this.handleConnectionError(error, reject);
        };

        this.ws.onclose = (event) => {
            this.handleDisconnect(event.code, event.reason);
        };
    }

    private handleMessage(message: LobbyMessage | LobbyMessage[]) {
        const messages = Array.isArray(message) ? message : [message];

        for (const msg of messages) {
            // Update internal state first
            this.updateState(msg);

            // Notify specific handlers
            const handlers = this.messageHandlers.get(msg.c) || [];
            handlers.forEach(handler => handler(msg));

            // Notify general listeners
            this.notifyGeneralListeners(msg);
        }
    }

    private updateState(message: LobbyMessage) {
        switch (message.c) {
            case MESSAGE_TYPES.PEER_STATE:
                this.peerState = message.d?.peer;
                break;
            case MESSAGE_TYPES.LOBBY_CREATED:
            case MESSAGE_TYPES.JOINED_LOBBY:
                this.lobbyState = message.d?.lobby;
                break;
            case MESSAGE_TYPES.LOBBY_LEFT:
                this.lobbyState = null;
                break;
            case MESSAGE_TYPES.PEER_JOINED:
                if (this.lobbyState && message.d?.peer) {
                    this.lobbyState.peers = this.lobbyState.peers || [];
                    this.lobbyState.peers.push(message.d.peer);
                }
                break;
            case MESSAGE_TYPES.PEER_LEFT:
                if (this.lobbyState && message.d?.peer_id) {
                    this.lobbyState.peers = (this.lobbyState.peers || []).filter(
                        p => p.id !== message.d.peer_id
                    );
                }
                break;
            // Add more state updates as needed
        }
    }

    private notifyGeneralListeners(message: LobbyMessage) {
        switch (message.c) {
            case MESSAGE_TYPES.LOBBY_CREATED:
            case MESSAGE_TYPES.JOINED_LOBBY:
            case MESSAGE_TYPES.LOBBY_TAGS:
            case MESSAGE_TYPES.LOBBY_SEALED:
            case MESSAGE_TYPES.LOBBY_UNSEALED:
                if (this.lobbyState) {
                    this.onLobbyUpdate?.(this.lobbyState);
                }
                break;
            case MESSAGE_TYPES.PEER_READY:
            case MESSAGE_TYPES.PEER_UNREADY:
            case MESSAGE_TYPES.PEER_USER_DATA:
                if (message.d?.peer) {
                    this.onPeerUpdate?.(message.d.peer);
                }
                break;
        }
    }

    private handleConnectionError(error: Error, reject?: (reason?: any) => void) {
        this.onError?.(error);
        if (reject) {
            reject(error);
            this.connectionPromise = null;
        }

        if (this.options.autoReconnect && 
            this.reconnectAttempts < this.options.maxReconnectAttempts) {
            this.reconnectAttempts++;
            setTimeout(() => this.connect(), this.options.reconnectInterval);
        }
    }

    private handleDisconnect(code: number, reason: string) {
        this.onDisconnect?.(reason);
        this.connectionPromise = null;

        if (this.options.autoReconnect && 
            this.reconnectAttempts < this.options.maxReconnectAttempts) {
            this.reconnectAttempts++;
            setTimeout(() => this.connect(), this.options.reconnectInterval);
        }
    }

    public disconnect(): void {
        if (this.ws) {
            this.ws.close();
            this.ws = null;
        }
        this.connectionPromise = null;
    }

    public sendCommand(command: string, data?: any): Promise<LobbyMessage> {
        return new Promise((resolve, reject) => {
            if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
                reject(new Error('WebSocket is not connected'));
                return;
            }

            const message: LobbyMessage = { 
                c: this.getCommandCode(command), 
                d: data 
            };

            // Set up a one-time handler for the response
            const expectedResponseCode = this.getExpectedResponseCode(command);
            const handler = (response: LobbyMessage) => {
                resolve(response);
            };

            this.onMessage(expectedResponseCode, handler, 5000); // 5 second timeout

            this.ws.send(JSON.stringify(message));
        });
    }

    public onMessage(
        messageCode: MessageCode, 
        handler: (msg: LobbyMessage) => void, 
        timeout = 0
    ): void {
        const handlers = this.messageHandlers.get(messageCode) || [];
        handlers.push(handler);
        this.messageHandlers.set(messageCode, handlers);

        if (timeout > 0) {
            setTimeout(() => {
                const handlers = this.messageHandlers.get(messageCode) || [];
                const index = handlers.indexOf(handler);
                if (index !== -1) {
                    handlers.splice(index, 1);
                    this.messageHandlers.set(messageCode, handlers);
                }
            }, timeout);
        }
    }

    // High-level API methods
    public async createLobby(options: { maxPlayers: number; name?: string }): Promise<Lobby> {
        const response = await this.sendCommand('create_lobby', {
            m: options.maxPlayers,
            name: options.name
        });
        return response.d?.lobby;
    }

    public async joinLobby(lobbyId: string): Promise<Lobby> {
        const response = await this.sendCommand('join_lobby', { lobby_id: lobbyId });
        return response.d?.lobby;
    }

    public async leaveLobby(): Promise<void> {
        await this.sendCommand('leave_lobby');
    }

    public async setReady(ready: boolean): Promise<void> {
        await this.sendCommand(ready ? 'lobby_ready' : 'lobby_unready');
    }

    public async setLobbyTags(tags: Record<string, any>): Promise<void> {
        await this.sendCommand('lobby_tags', { tags });
    }

    public async sealLobby(sealed: boolean): Promise<void> {
        await this.sendCommand(sealed ? 'seal_lobby' : 'unseal_lobby');
    }

    public async kickPeer(peerId: string): Promise<void> {
        await this.sendCommand('kick_peer', { peer: peerId });
    }

    public async listLobbies(): Promise<Lobby[]> {
        const response = await this.sendCommand('list_lobby');
        return response.d?.lobbies || [];
    }

    // Helper methods
    private getCommandCode(command: string): MessageCode {
        const commandMap: Record<string, MessageCode> = {
            'create_lobby': 1,
            'join_lobby': 2,
            'leave_lobby': 3,
            'lobby_ready': 8,
            'lobby_unready': 9,
            'seal_lobby': 4,
            'unseal_lobby': 3, // Note: This might need adjustment based on your protocol
            'lobby_tags': 16,
            'kick_peer': 12,
            'list_lobby': 22,
        };
        return commandMap[command] || MESSAGE_TYPES.ERROR;
    }

    private getExpectedResponseCode(command: string): MessageCode {
        const responseMap: Record<string, MessageCode> = {
            'create_lobby': MESSAGE_TYPES.LOBBY_CREATED,
            'join_lobby': MESSAGE_TYPES.JOINED_LOBBY,
            'leave_lobby': MESSAGE_TYPES.LOBBY_LEFT,
            'lobby_ready': MESSAGE_TYPES.PEER_READY,
            'lobby_unready': MESSAGE_TYPES.PEER_UNREADY,
            'seal_lobby': MESSAGE_TYPES.LOBBY_SEALED,
            'unseal_lobby': MESSAGE_TYPES.LOBBY_UNSEALED,
            'lobby_tags': MESSAGE_TYPES.LOBBY_TAGS,
            'kick_peer': MESSAGE_TYPES.PEER_LEFT,
            'list_lobby': MESSAGE_TYPES.LOBBY_LIST,
        };
        return responseMap[command] || MESSAGE_TYPES.ERROR;
    }

    // State getters
    public getPeerState(): Peer | null {
        return this.peerState;
    }

    public getLobbyState(): Lobby | null {
        return this.lobbyState;
    }

    public getReconnectionToken(): string | undefined {
        return this.peerState?.reconnection_token;
    }

    public isConnected(): boolean {
        return this.ws?.readyState === WebSocket.OPEN;
    }
}

// Export the MESSAGE_TYPES for external use
export { LobbyClient, MESSAGE_TYPES };
