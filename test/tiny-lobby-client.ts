// ======================
// Public Types (Exported)
// ======================

export interface Lobby {
    readonly id: string;
    readonly name: string;
    readonly hostId: string;
    readonly isSealed: boolean;
    readonly maxPlayers: number;
    readonly hasPassword: boolean;
    readonly createdAt: number;
    readonly tags: Record<string, any>;
    readonly publicData: Record<string, any>;
    readonly privateData?: Record<string, any>;
    readonly peerCount: number;
}

export interface Peer {
    readonly id: string;
    readonly orderId: number;
    readonly userData: Record<string, any>;
    readonly isReady: boolean;
    readonly publicData: Record<string, any>;
    readonly isDisconnected: boolean;
    readonly platform: string;
}

export interface SelfPeer extends Peer {
    readonly privateData: Record<string, any>;
    readonly reconnectionToken: string;
}

export interface ChatMessage {
    readonly fromPeerId: string;
    readonly content: string;
    readonly metadata: Record<string, any>;
}

export enum LobbyEvent {
    // Connection events
    CONNECTED = 'connected',
    DISCONNECTED = 'disconnected',
    
    // Lobby events
    LOBBY_CREATED = 'lobby-created',
    LOBBY_JOINED = 'lobby-joined',
    LOBBY_LEFT = 'lobby-left',
    LOBBY_SEALED = 'lobby-sealed',
    LOBBY_HOSTED = 'lobby-hosted',
    LOBBY_TAGS_UPDATED = 'lobby-tags-updated',
    LOBBY_PASSWORD_CHANGED = 'lobby-password-changed',
    LOBBY_RESIZED = 'lobby-resized',
    LOBBY_TITLE_CHANGED = 'lobby-title-changed',
    LOBBY_DATA_CHANGED = 'lobby-data-changed',
    LOBBY_LIST = 'lobby-list',
    
    // Peer events
    PEER_JOINED = 'peer-joined',
    PEER_LEFT = 'peer-left',
    PEER_RECONNECTED = 'peer-reconnected',
    PEER_DISCONNECTED = 'peer-disconnected',
    PEER_READY_CHANGED = 'peer-ready-changed',
    PEER_USER_DATA_CHANGED = 'peer-user-data-changed',
    PEER_DATA_CHANGED = 'peer-data-changed',
    
    // Chat events
    CHAT_MESSAGE = 'chat-message',
    
    // Notification events
    NOTIFICATION = 'notification',
    
    // Error events
    ERROR = 'error',
    LOG = 'log'
}

export class TinyLobbyError extends Error {
    constructor(public readonly code: string, message: string) {
        super(message);
        this.name = "TinyLobbyError";
    }
}

// ======================
// Internal Types
// ======================

type Callback<T> = (data: T) => void;

interface InternalMessageBase {
    c: number;
    d: any;
    m?: string;
    e?: boolean;
}

enum InternalCommand {
    LOBBY_DATA = 0,
    LOBBY_DATA_TO = 1,
    LOBBY_DATA_TO_ALL = 2,
    LOBBY_NOTIFY_TO = 3,
    LOBBY_NOTIFY = 4,
    LOBBY_CALL = 5,
    LOBBY_QUICK_JOIN = 6,
    CREATE_LOBBY = 7,
    JOIN_LOBBY = 8,
    LEAVE_LOBBY = 9,
    LIST_LOBBY = 10,
    CHAT_LOBBY = 11,
    LOBBY_TAGS = 12,
    KICK_PEER = 13,
    USER_DATA = 14,
    LOBBY_READY = 15,
    LOBBY_UNREADY = 16,
    LOBBY_SEAL = 17,
    LOBBY_UNSEAL = 18,
    LOBBY_MAX_PLAYERS = 19,
    LOBBY_TITLE = 20,
    LOBBY_PASSWORD = 21,
}

enum InternalResponse {
    ERROR = 0,
    LOBBY_HOSTED = 1,
    LOBBY_CREATED = 2,
    LOBBY_UNSEALED = 3,
    LOBBY_SEALED = 4,
    LOBBY_RESIZED = 5,
    LOBBY_PASSWORDED = 6,
    LOBBY_TITLED = 7,
    PEER_READY = 8,
    PEER_UNREADY = 9,
    LOBBY_LEFT = 10,
    PEER_LEFT = 11,
    LOBBY_KICKED = 12,
    PEER_STATE = 13,
    LOBBY_CALL = 14,
    PEER_USER_DATA = 15,
    LOBBY_TAGS = 16,
    PEER_CHAT = 17,
    PEER_RECONNECTED = 18,
    JOINED_LOBBY = 19,
    PEER_JOINED = 20,
    PEER_DISCONNECTED = 21,
    LOBBY_LIST = 22,
    LOBBY_DATA = 23,
    PEER_NOTIFY = 24,
    DATA_TO = 25,
    DATA_TO_SENT = 26,
    NOTIFY_TO_SENT = 27,
}

interface InternalLobbyModel {
    id: string;
    n: string;
    h: string;
    s: boolean;
    m: number;
    p: string[];
    _p: boolean;
    c: number;
    t: Record<string, any>;
    d: Record<string, any>;
    _d: Record<string, any>;
}

interface InternalPeerModel {
    id: string;
    oi: number;
    ud: Record<string, any>;
    r: boolean;
    l: string;
    d: Record<string, any>;
    dc: boolean;
    p: string;
    rt: string;
    _d: Record<string, any>;
}

interface TinyLobbyOptions {
    gameId: string;
    reconnectionToken?: string;
    url?: string;
    debug?: boolean;
    timeout?: number;
}

// ======================
// Client Implementation
// ======================

export class TinyLobbyClient {
    private socket: WebSocket | null = null;
    private messageIdCounter: number = 0;
    private responseHandlers = new Map<number, {
        resolve: (value: any) => void,
        reject: (reason?: any) => void
    }>();
    private eventHandlers = new Map<LobbyEvent, Callback<any>[]>();

    // Client state
    private _lobby: Lobby | null = null;
    private _selfPeer: SelfPeer | null = null;
    private _peers: Map<string, Peer> = new Map();
    private _lobbies: Lobby[] = [];
    private _connected: boolean = false;
    private _isHost: boolean = false;

    get lobby(): Lobby | null {
        return this._lobby;
    }

    get selfPeer(): SelfPeer | null {
        return this._selfPeer;
    }

    get peers(): Peer[] {
        return Array.from(this._peers.values());
    }

    get peerCount(): number {
        return this._peers.size;
    }

    get lobbies(): Lobby[] {
        return this._lobbies;
    }

    get connected(): boolean {
        return this._connected;
    }

    get isHost(): boolean {
        return this._isHost;
    }

    constructor(private options: TinyLobbyOptions) {
        if (!options.url) {
            options.url = "wss://lobby.appsinacup.com/connect";
        }
        if (!options.timeout) {
            options.timeout = 5000;
        }
    }

    // ======================
    // Connection Management
    // ======================

    async connect(): Promise<void> {
        return new Promise((resolve, reject) => {
            const protocols = ['appsinacup', this.options.gameId];
            if (this.options.reconnectionToken) {
                protocols.push(this.options.reconnectionToken);
            }

            this.socket = new WebSocket(this.options.url!, protocols);

            this.socket.onopen = () => {
                this._connected = true;
                if (this.options.debug) {
                    this.log("Connected to server", "log");
                }
                resolve();
            };

            this.socket.onerror = (error) => {
                if (this.options.debug) {
                    this.log("Connection error", "error");
                }
                reject(new TinyLobbyError("connection-failed", "Failed to connect to server"));
            };

            this.socket.onclose = (event) => {
                this._connected = false;
                this.clearState();
                
                if (this.options.debug) {
                    this.log(`Disconnected: ${event.code} ${event.reason}`, "disconnected");
                }
                
                this.emit(LobbyEvent.DISCONNECTED, { 
                    code: event.code, 
                    reason: event.reason 
                });
            };

            this.socket.onmessage = (event) => {
                try {
                    const message = JSON.parse(event.data) as InternalMessageBase | InternalMessageBase[];
                    
                    if (Array.isArray(message)) {
                        message.forEach(msg => this.handleMessage(msg));
                    } else {
                        this.handleMessage(message);
                    }
                } catch (error) {
                    if (this.options.debug) {
                        this.log("Error parsing message", "error");
                    }
                    this.emit(LobbyEvent.ERROR, new TinyLobbyError("invalid-message", "Failed to parse server message"));
                }
            };
        });
    }

    disconnect(): void {
        if (this.socket) {
            this.socket.close();
            this.socket = null;
        }
        this.clearState();
        this.responseHandlers.clear();
    }

    private clearState(): void {
        this._lobby = null;
        this._selfPeer = null;
        this._peers.clear();
        this._lobbies = [];
        this._isHost = false;
    }

    // ======================
    // Message Handling
    // ======================

    private handleMessage(message: InternalMessageBase) {
        if (this.options.debug) {
            this.log(`Received message: ${JSON.stringify(message)}`, "message");
        }

        // Check if this is a response to a specific command
        if (message.d?.id !== undefined) {
            const handler = this.responseHandlers.get(message.d.id);
            if (handler) {
                if (message.c === InternalResponse.ERROR) {
                    const error = new TinyLobbyError(message.m || "unknown-error", message.m || "Unknown error");
                    handler.reject(error);
                    this.emit(LobbyEvent.ERROR, error);
                } else {
                    handler.resolve(message);
                }
                this.responseHandlers.delete(message.d.id);
                return;
            }
        }

        // Handle state updates and events
        switch (message.c) {
            case InternalResponse.PEER_STATE:
                this.handlePeerState(message.d.p);
                break;
                
            case InternalResponse.JOINED_LOBBY:
                this.handleJoinedLobby(message.d);
                break;
                
            case InternalResponse.LOBBY_CREATED:
                this.handleLobbyCreated(message.d);
                break;
                
            case InternalResponse.LOBBY_LEFT:
                this.handleLobbyLeft(false);
                break;
                
            case InternalResponse.LOBBY_KICKED:
                this.handleLobbyLeft(true);
                break;
                
            case InternalResponse.LOBBY_SEALED:
                this.handleLobbySealed(true);
                break;
                
            case InternalResponse.LOBBY_UNSEALED:
                this.handleLobbySealed(false);
                break;
                
            case InternalResponse.LOBBY_HOSTED:
                this.handleLobbyHosted(message.d.h);
                break;
                
            case InternalResponse.LOBBY_TAGS:
                this.handleLobbyTags(message.d.t);
                break;
                
            case InternalResponse.LOBBY_PASSWORDED:
                this.handleLobbyPasswordChanged(message.d._p);
                break;
                
            case InternalResponse.LOBBY_RESIZED:
                this.handleLobbyResized(message.d.m);
                break;
                
            case InternalResponse.LOBBY_TITLED:
                this.handleLobbyTitleChanged(message.d.n);
                break;
                
            case InternalResponse.LOBBY_LIST:
                this.handleLobbyList(message.d.l);
                break;
                
            case InternalResponse.LOBBY_DATA:
                this.handleLobbyData(message.d.d, message.d._p);
                break;
                
            case InternalResponse.PEER_JOINED:
                this.handlePeerJoined(message.d.p);
                break;
                
            case InternalResponse.PEER_LEFT:
                this.handlePeerLeft(message.d.p, message.d.k);
                break;
                
            case InternalResponse.PEER_RECONNECTED:
                this.handlePeerReconnected(message.d.p);
                break;
                
            case InternalResponse.PEER_DISCONNECTED:
                this.handlePeerDisconnected(message.d.p);
                break;
                
            case InternalResponse.PEER_READY:
                this.handlePeerReady(message.d.p, true);
                break;
                
            case InternalResponse.PEER_UNREADY:
                this.handlePeerReady(message.d.p, false);
                break;
                
            case InternalResponse.PEER_USER_DATA:
                this.handlePeerUserData(message.d.p, message.d.ud);
                break;
                
            case InternalResponse.PEER_CHAT:
                this.handleChatMessage({
                    fromPeerId: message.d.f,
                    content: message.d.c,
                    metadata: message.d.m
                });
                break;
                
            case InternalResponse.PEER_NOTIFY:
                this.handleNotification(message.d.d);
                break;
                
            case InternalResponse.DATA_TO:
                this.handlePeerData(
                    message.d.p,
                    message.d.tp,
                    message.d.d,
                    message.d._p
                );
                break;
                
            case InternalResponse.LOBBY_CALL:
                // Handled by response handler
                break;
                
            default:
                if (this.options.debug) {
                    this.log(`Unknown message type: ${message.c}`, "warning");
                }
        }
    }

    private async sendCommand<T>(command: InternalCommand, data: any, timeout?: number): Promise<T> {
        if (!this.socket || this.socket.readyState !== WebSocket.OPEN) {
            throw new TinyLobbyError("not-connected", "WebSocket is not connected");
        }

        const messageId = this.messageIdCounter++;
        const message = {
            c: command,
            d: {
                ...data,
                id: messageId
            }
        };

        return new Promise<T>((resolve, reject) => {
            const timeoutId = setTimeout(() => {
                this.responseHandlers.delete(messageId);
                const error = new TinyLobbyError("timeout", "Request timed out");
                reject(error);
                this.emit(LobbyEvent.ERROR, error);
            }, timeout || this.options.timeout!);

            this.responseHandlers.set(messageId, {
                resolve: (response: any) => {
                    clearTimeout(timeoutId);
                    resolve(response);
                },
                reject: (error: any) => {
                    clearTimeout(timeoutId);
                    reject(error);
                }
            });

            this.socket!.send(JSON.stringify(message));

            if (this.options.debug) {
                this.log(`Sent command: ${JSON.stringify(message)}`, "command");
            }
        });
    }

    // ======================
    // State Update Handlers
    // ======================

    private handlePeerState(peerData: InternalPeerModel) {
        const isSelf = !this._selfPeer || peerData.id === this._selfPeer.id;
        
        if (isSelf) {
            // Update self peer
            this._selfPeer = this.transformSelfPeer(peerData);
            this._isHost = this._lobby?.hostId === this._selfPeer.id;
            
            if (!this._connected) {
                this._connected = true;
                this.emit(LobbyEvent.CONNECTED, {
                    peer: this._selfPeer,
                    reconnectionToken: this._selfPeer.reconnectionToken
                });
            }
        } else {
            // Update other peer
            const peer = this.transformPeer(peerData);
            this._peers.set(peer.id, peer);
        }
    }

    private handleJoinedLobby(data: { l: InternalLobbyModel, p: InternalPeerModel[] }) {
        this._lobby = this.transformLobby(data.l);
        
        // Update self peer from the first peer in the list (should be us)
        if (data.p.length > 0) {
            this._selfPeer = this.transformSelfPeer(data.p[0]);
            this._isHost = this._lobby.hostId === this._selfPeer.id;
        }
        
        // Update peers list
        this._peers.clear();
        for (const peer of data.p) {
            if (peer.id !== this._selfPeer?.id) {
                this._peers.set(peer.id, this.transformPeer(peer));
            }
        }
        
        this.emit(LobbyEvent.LOBBY_JOINED, {
            lobby: this._lobby,
            peers: this.peers
        });
    }

    private handleLobbyCreated(data: { l: InternalLobbyModel, p: InternalPeerModel[] }) {
        this._lobby = this.transformLobby(data.l);
        
        // We should be the only peer when creating a lobby
        if (data.p.length > 0) {
            this._selfPeer = this.transformSelfPeer(data.p[0]);
            this._isHost = true;
        }
        
        this._peers.clear();
        this.emit(LobbyEvent.LOBBY_CREATED, {
            lobby: this._lobby,
            peers: this.peers
        });
    }

    private handleLobbyLeft(wasKicked: boolean) {
        this.clearState();
        this.emit(LobbyEvent.LOBBY_LEFT, { wasKicked });
    }

    private handleLobbySealed(isSealed: boolean) {
        if (this._lobby) {
            this._lobby = {
                ...this._lobby,
                isSealed
            };
            this.emit(LobbyEvent.LOBBY_SEALED, { isSealed });
        }
    }

    private handleLobbyHosted(newHostId: string) {
        if (this._lobby) {
            this._lobby = {
                ...this._lobby,
                hostId: newHostId
            };
            
            this._isHost = this._selfPeer?.id === newHostId;
            
            const newHost = this._peers.get(newHostId) || this._selfPeer;
            if (newHost) {
                this.emit(LobbyEvent.LOBBY_HOSTED, { host: newHost });
            }
        }
    }

    private handleLobbyTags(tags: Record<string, any>) {
        if (this._lobby) {
            this._lobby = {
                ...this._lobby,
                tags
            };
            this.emit(LobbyEvent.LOBBY_TAGS_UPDATED, { tags });
        }
    }

    private handleLobbyPasswordChanged(hasPassword: boolean) {
        if (this._lobby) {
            this._lobby = {
                ...this._lobby,
                hasPassword
            };
            this.emit(LobbyEvent.LOBBY_PASSWORD_CHANGED, { hasPassword });
        }
    }

    private handleLobbyResized(maxPlayers: number) {
        if (this._lobby) {
            this._lobby = {
                ...this._lobby,
                maxPlayers
            };
            this.emit(LobbyEvent.LOBBY_RESIZED, { maxPlayers });
        }
    }

    private handleLobbyTitleChanged(title: string) {
        if (this._lobby) {
            this._lobby = {
                ...this._lobby,
                name: title
            };
            this.emit(LobbyEvent.LOBBY_TITLE_CHANGED, { title });
        }
    }

    private handleLobbyList(lobbiesData: InternalLobbyModel[]) {
        this._lobbies = lobbiesData.map(lobby => this.transformLobby(lobby));
        this.emit(LobbyEvent.LOBBY_LIST, { lobbies: this._lobbies });
    }

    private handleLobbyData(data: Record<string, any>, isPrivate: boolean) {
        if (this._lobby) {
            this._lobby = {
                ...this._lobby,
                publicData: isPrivate ? this._lobby.publicData : data,
                ...(isPrivate ? { privateData: data } : {})
            };
            this.emit(LobbyEvent.LOBBY_DATA_CHANGED, { data, isPrivate });
        }
    }

    private handlePeerJoined(peerId: string) {
        // In a real implementation, we would fetch the full peer data
        // For now, we'll just add a placeholder
        const newPeer: Peer = {
            id: peerId,
            orderId: 0,
            userData: {},
            isReady: false,
            publicData: {},
            isDisconnected: false,
            platform: 'unknown'
        };
        
        this._peers.set(peerId, newPeer);
        
        if (this._lobby) {
            this._lobby = {
                ...this._lobby,
                peerCount: this._peers.size + (this._selfPeer ? 1 : 0)
            };
        }
        
        this.emit(LobbyEvent.PEER_JOINED, { peer: newPeer });
    }

    private handlePeerLeft(peerId: string, wasKicked: boolean) {
        const peer = this._peers.get(peerId);
        if (peer) {
            this._peers.delete(peerId);
            
            if (this._lobby) {
                this._lobby = {
                    ...this._lobby,
                    peerCount: this._peers.size + (this._selfPeer ? 1 : 0)
                };
            }
            
            this.emit(LobbyEvent.PEER_LEFT, { peer, wasKicked });
        }
    }

    private handlePeerReconnected(peerId: string) {
        const peer = this._peers.get(peerId);
        if (peer) {
            const updatedPeer: Peer = {
                ...peer,
                isDisconnected: false
            };
            this._peers.set(peerId, updatedPeer);
            this.emit(LobbyEvent.PEER_RECONNECTED, { peer: updatedPeer });
        }
    }

    private handlePeerDisconnected(peerId: string) {
        const peer = this._peers.get(peerId);
        if (peer) {
            const updatedPeer: Peer = {
                ...peer,
                isDisconnected: true
            };
            this._peers.set(peerId, updatedPeer);
            this.emit(LobbyEvent.PEER_DISCONNECTED, { peer: updatedPeer });
        }
    }

    private handlePeerReady(peerId: string, isReady: boolean) {
        if (this._selfPeer?.id === peerId) {
            this._selfPeer = {
                ...this._selfPeer,
                isReady
            };
        }
        
        const peer = this._peers.get(peerId);
        if (peer) {
            const updatedPeer: Peer = {
                ...peer,
                isReady
            };
            this._peers.set(peerId, updatedPeer);
            this.emit(LobbyEvent.PEER_READY_CHANGED, { peer: updatedPeer, isReady });
        }
    }

    private handlePeerUserData(peerId: string, userData: Record<string, any>) {
        if (this._selfPeer?.id === peerId) {
            this._selfPeer = {
                ...this._selfPeer,
                userData
            };
        }
        
        const peer = this._peers.get(peerId);
        if (peer) {
            const updatedPeer: Peer = {
                ...peer,
                userData
            };
            this._peers.set(peerId, updatedPeer);
        }
        
        this.emit(LobbyEvent.PEER_USER_DATA_CHANGED, { peerId, userData });
    }

    private handleChatMessage(message: ChatMessage) {
        this.emit(LobbyEvent.CHAT_MESSAGE, message);
    }

    private handleNotification(data: Record<string, any>) {
        this.emit(LobbyEvent.NOTIFICATION, { data });
    }

    private handlePeerData(
        fromPeerId: string,
        targetPeerId: string,
        data: Record<string, any>,
        isPrivate: boolean
    ) {
        if (isPrivate && targetPeerId === this._selfPeer?.id) {
            // Update our private data
            this._selfPeer = {
                ...this._selfPeer,
                privateData: data
            };
        }
        
        if (!isPrivate) {
            if (targetPeerId === this._selfPeer?.id) {
                // Update our public data
                this._selfPeer = {
                    ...this._selfPeer,
                    publicData: data
                };
            } else {
                // Update other peer's public data
                const peer = this._peers.get(targetPeerId);
                if (peer) {
                    const updatedPeer: Peer = {
                        ...peer,
                        publicData: data
                    };
                    this._peers.set(targetPeerId, updatedPeer);
                }
            }
        }
        
        const fromPeer = this._peers.get(fromPeerId) || this._selfPeer;
        const toPeer = this._peers.get(targetPeerId) || this._selfPeer;
        
        if (fromPeer && toPeer) {
            this.emit(LobbyEvent.PEER_DATA_CHANGED, {
                fromPeer,
                toPeer,
                data,
                isPrivate
            });
        }
    }

    // ======================
    // Model Transformations
    // ======================

    private transformLobby(lobby: InternalLobbyModel): Lobby {
        return {
            id: lobby.id,
            name: lobby.n,
            hostId: lobby.h,
            isSealed: lobby.s,
            maxPlayers: lobby.m,
            hasPassword: lobby._p,
            createdAt: lobby.c,
            tags: lobby.t,
            publicData: lobby.d,
            privateData: lobby._d,
            peerCount: lobby.p.length
        };
    }

    private transformPeer(peer: InternalPeerModel): Peer {
        return {
            id: peer.id,
            orderId: peer.oi,
            userData: peer.ud,
            isReady: peer.r,
            publicData: peer.d,
            isDisconnected: peer.dc,
            platform: peer.p
        };
    }

    private transformSelfPeer(peer: InternalPeerModel): SelfPeer {
        return {
            ...this.transformPeer(peer),
            privateData: peer._d,
            reconnectionToken: peer.rt
        };
    }

    // ======================
    // Event Management
    // ======================

    on(event: LobbyEvent, callback: Callback<any>): void {
        if (!this.eventHandlers.has(event)) {
            this.eventHandlers.set(event, []);
        }
        this.eventHandlers.get(event)!.push(callback);
    }

    off(event: LobbyEvent, callback: Callback<any>): void {
        const callbacks = this.eventHandlers.get(event);
        if (callbacks) {
            const index = callbacks.indexOf(callback);
            if (index !== -1) {
                callbacks.splice(index, 1);
            }
        }
    }

    private emit(event: LobbyEvent, data?: any): void {
        const callbacks = this.eventHandlers.get(event);
        if (callbacks) {
            // Clone data to prevent modification
            const eventData = typeof data === 'object' ? {...data} : data;
            callbacks.forEach(cb => cb(eventData));
        }
        
        if (this.options.debug) {
            this.log(`Event: ${event}`, "event", data);
        }
    }

    private log(message: string, type: 'log' | 'error' | 'warning' | 'command' | 'message' | 'event' | 'disconnected', data?: any) {
        console[type === 'error' ? 'error' : 'log'](`[TinyLobby] ${type.toUpperCase()}: ${message}`);
        if (data) {
            console.log(data);
        }
    }

    // ======================
    // Public API Methods
    // ======================

    async quickJoin(name: string, tags: Record<string, any> = {}, maxPlayers: number = 4): Promise<Lobby> {
        const response = await this.sendCommand(InternalCommand.LOBBY_QUICK_JOIN, {
            n: name,
            m: maxPlayers,
            t: tags
        });
        return this._lobby!;
    }

    async createLobby(name: string, options: {
        sealed?: boolean;
        tags?: Record<string, any>;
        maxPlayers?: number;
        password?: string;
    } = {}): Promise<Lobby> {
        const response = await this.sendCommand(InternalCommand.CREATE_LOBBY, {
            n: name,
            s: options.sealed || false,
            t: options.tags || {},
            m: options.maxPlayers || 4,
            _p: options.password || ""
        });
        return this._lobby!;
    }

    async joinLobby(lobbyId: string, password?: string): Promise<Lobby> {
        const response = await this.sendCommand(InternalCommand.JOIN_LOBBY, {
            id: lobbyId,
            _p: password || ""
        });
        return this._lobby!;
    }

    async leaveLobby(): Promise<void> {
        await this.sendCommand(InternalCommand.LEAVE_LOBBY, {});
    }

    async listLobbies(): Promise<Lobby[]> {
        const response = await this.sendCommand(InternalCommand.LIST_LOBBY, {});
        return this._lobbies;
    }

    async setMaxPlayers(maxPlayers: number): Promise<void> {
        await this.sendCommand(InternalCommand.LOBBY_MAX_PLAYERS, {
            m: maxPlayers
        });
    }

    async setTitle(title: string): Promise<void> {
        await this.sendCommand(InternalCommand.LOBBY_TITLE, {
            n: title
        });
    }

    async setPassword(password: string): Promise<void> {
        await this.sendCommand(InternalCommand.LOBBY_PASSWORD, {
            _p: password
        });
    }

    async sealLobby(): Promise<void> {
        await this.sendCommand(InternalCommand.LOBBY_SEAL, {});
    }

    async unsealLobby(): Promise<void> {
        await this.sendCommand(InternalCommand.LOBBY_UNSEAL, {});
    }

    async setLobbyTags(tags: Record<string, any>): Promise<void> {
        await this.sendCommand(InternalCommand.LOBBY_TAGS, {
            t: tags
        });
    }

    async removeLobbyTags(tagKeys: string[]): Promise<void> {
        const tagsToRemove = tagKeys.reduce((acc, key) => {
            acc[key] = null;
            return acc;
        }, {} as Record<string, null>);
        
        await this.sendCommand(InternalCommand.LOBBY_TAGS, {
            t: tagsToRemove
        });
    }

    async setUserData(userData: Record<string, any>): Promise<void> {
        await this.sendCommand(InternalCommand.USER_DATA, {
            ud: userData
        });
    }

    async removeUserData(keys: string[]): Promise<void> {
        const dataToRemove = keys.reduce((acc, key) => {
            acc[key] = null;
            return acc;
        }, {} as Record<string, null>);
        
        await this.sendCommand(InternalCommand.USER_DATA, {
            ud: dataToRemove
        });
    }

    async setReady(isReady: boolean = true): Promise<void> {
        await this.sendCommand(
            isReady ? InternalCommand.LOBBY_READY : InternalCommand.LOBBY_UNREADY, 
            {}
        );
    }

    async kickPeer(peerId: string): Promise<void> {
        await this.sendCommand(InternalCommand.KICK_PEER, {
            p: peerId
        });
    }

    async sendChatMessage(content: string, metadata: Record<string, any> = {}): Promise<void> {
        await this.sendCommand(InternalCommand.CHAT_LOBBY, {
            c: content,
            m: metadata
        });
    }

    async callLobbyFunction(method: string, args: any[] = []): Promise<any> {
        const response: any = await this.sendCommand(InternalCommand.LOBBY_CALL, {
            f: method,
            i: args
        });
        return response.d.r;
    }

    async setLobbyData(data: Record<string, any>, isPrivate: boolean = false): Promise<void> {
        await this.sendCommand(InternalCommand.LOBBY_DATA, {
            d: data,
            _p: isPrivate
        });
    }

    async setPeerData(targetPeerId: string, data: Record<string, any>, isPrivate: boolean = false): Promise<void> {
        await this.sendCommand(InternalCommand.LOBBY_DATA_TO, {
            d: data,
            tp: targetPeerId,
            _p: isPrivate
        });
    }

    async setPeerDataToAll(data: Record<string, any>, isPrivate: boolean = false): Promise<void> {
        await this.sendCommand(InternalCommand.LOBBY_DATA_TO_ALL, {
            d: data,
            _p: isPrivate
        });
    }

    async notifyPeer(targetPeerId: string, data: Record<string, any>): Promise<void> {
        await this.sendCommand(InternalCommand.LOBBY_NOTIFY_TO, {
            d: data,
            tp: targetPeerId
        });
    }

    async notifyAll(data: Record<string, any>): Promise<void> {
        await this.sendCommand(InternalCommand.LOBBY_NOTIFY, {
            d: data
        });
    }
}
