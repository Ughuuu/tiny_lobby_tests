## Blazium Login Authentication

The Authentication Flow between client and login service and the authentication provider (discord/steam/etc.). The outcome of this step is the Session Key.

```mermaid
graph TD
    Client -->|1 Connect| LoginService
    Client -->|2 OAuth Flow| OAuthProvider
    OAuthProvider -->|3 Notifies| LoginService
    LoginService -->|4 Session Key| Client
```

## Blazium Lobby Connect

After the user is authenticated, he connects to the `LobbyService` as an authenticated user. Now the `LobbyService` can make calls on his behalf.

```mermaid
graph TD
    Client -->|1 Connect with Session Key| LobbyService
    LobbyService -->|Contact with User Info| Leaderboards
    LobbyService -->|Contact with User Info| Banlist
```
