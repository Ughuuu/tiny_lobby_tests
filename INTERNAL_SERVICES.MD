
## Blazium Arcade

This is the service that controls the other services: BlaziumLobby, BlaziumLogin, etc. and connects them.

- On Periodic Update

```mermaid
graph TD
    LobbyService -->|on_server_update lobby_server_id| ArcadeService
```

- On Startup

```mermaid
graph TD
    LobbyService -->|on_server_startup| ArcadeService
    ArcadeService -->|lobby_server_id| LobbyService
```

- Example of Lobby to LobbyServerID mapping:

```
   LobbyService1 --> LobbyServerID1
   LobbyService2 --> LobbyServerID2
   LobbyService3 --> LobbyServerID3
```

- Arcade Game Startup + Login Details

```mermaid
graph TD
    ArcadeService -->|arcade_game and arcade_lobby| LoginService
    LoginService -->|0 Notify with Session Info| LobbyService
    LoginService -->|1 Notify Connection Info| Client
    Client -->|2 Connect with Session Key| LobbyService
```
