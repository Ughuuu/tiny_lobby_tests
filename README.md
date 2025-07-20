# Tiny Lobby

|[Website](https://appsinacup.com)|[Discord](https://discord.gg/56dMud8HYn)|[Documentation](https://github.com/appsinacup/documentation_lobby)|[Build Locally](./BUILD_LOCALLY.md)
|-|-|-|-|

![example](docs/example.gif)

Multiplayer C++ Lobby Server with login for create / join / find lobbies. It starts a websocket server and has backend scripting in Luau and AngelScript.

- [addon_tiny_lobby_client](https://github.com/appsinacup/addon_tiny_lobby_client): Godot Tiny Lobby Client

## Usage

Run locally by downloading latest [GitHub Release](https://github.com/appsinacup/tiny_lobby/releases) and running it in terminal:

```sh
tiny_lobby
```

Or start it with docker by running:

```sh
docker pull ghcr.io/appsinacup/tiny_lobby:latest
docker run -p 8080:8080 ghcr.io/appsinacup/tiny_lobby:latest
```

For more info go to the [Tiny Lobby Documentation](https://github.com/appsinacup/documentation_lobby) page.
