```mermaid
graph TD
    MainThread -->|Init & Config| WebSocketServer
    WebSocketServer -->|Auth Queue| WebAuthThread
    WebSocketServer -->|Receive Queue| GameThread
    MainThread -->|Analytics Queue| POGRClient
    MainThread --> GameThread
    GameThread -->|Send Queue| WebSocketServer
```
