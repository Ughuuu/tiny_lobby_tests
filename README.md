# Tiny Lobby

|[Website](https://appsinacup.com)|[Discord](https://discord.gg/56dMud8HYn)|
|-|-|

This is the C++ implementation of the Lobby Websocket Server. It starts a websocket server and has backend scripting in Luau and AngelScript.

- [addon_tiny_lobby_client](https://github.com/appsinacup/addon_tiny_lobby_client): Godot Tiny Lobby Client

## Requirements

- clang version 16.0.0
- cmake version 3.30.5
- Run `install.sh`

## How to build

1. MacOS

```sh
./bash/install-macos.sh
```

2. Linux

```sh
./bash/install-linux.sh
```

3. Windows

```sh
./bash/install-windows.bat
```

Run `./bash/build.sh` and then run `build/lobby_server`.
