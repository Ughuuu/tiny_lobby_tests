# Blazium Lobby Server

This is the C++ implementation of the Blazium Web Server. It uses [uWebSockets](https://github.com/uNetworking/uWebSockets/tree/master)

## Requirements

- Install boost
- Install zlib
- Run `install.sh`

1. MacOS

```sh
brew install zlib-devel
brew install boost
```

2. Linux

```sh
sudo apt-get update
sudo apt-get install -y build-essential
sudo apt install libboost-uuid-dev libboost-system-dev
```

## How to build

Run `build.sh` and then run `build/lobby_server`
