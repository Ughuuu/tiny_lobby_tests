# Tiny Lobby Server
Tiny Lobby is a C++ multiplayer lobby server with WebSocket communication and Lua scripting support. It uses CMake with vcpkg dependency management and supports cross-platform builds.

Always reference these instructions first and fallback to search or bash commands only when you encounter unexpected information that does not match the info here.

## Working Effectively
- **CRITICAL: NETWORK DEPENDENCY ISSUE**: vcpkg installation of libpqxx may fail due to PostgreSQL FTP server connectivity issues. If `./vcpkg install libpqxx` fails with DNS resolution or timeout errors for `ftp.postgresql.org`, install system packages instead: `sudo apt-get install -y libpq-dev libpqxx-dev`
- Bootstrap dependencies and build the repository:
  - `git submodule update --init --recursive`
  - `sudo apt-get update && sudo apt-get install -y build-essential zlib1g-dev g++-8 cmake libbz2-dev libjson-c-dev python-minimal`
  - `./bash/install-linux-x64.sh` -- takes 15+ minutes. NEVER CANCEL. Set timeout to 30+ minutes.
  - If vcpkg libpqxx installation fails: `sudo apt-get install -y libpq-dev libpqxx-dev`
- Build release version:
  - `./bash/build_release-x64.sh` -- takes 2-5 minutes when dependencies are ready. Set timeout to 15+ minutes.
- Test suite (Node.js based):
  - `cd test && npm install` -- takes 30 seconds. Set timeout to 2+ minutes.
  - `npm run test` -- runs performance tests against server
  - `npm run unit` -- runs unit tests with mocha

## Validation
- **NEVER CANCEL builds or dependency installations** - vcpkg dependency installation takes 15+ minutes, builds take 2-5 minutes
- Always run the full dependency installation first before attempting builds
- Database functionality is disabled by default (`config.ini` has `enabled=false`)
- Always run `./bash/format.sh` and `./bash/check_format.sh` before committing (CI requirement)
- Server runs on port 8080 by default with WebSocket protocol

## Common Build Issues
- **libpqxx network failure**: Most common issue. Install system packages if vcpkg fails: `sudo apt-get install -y libpq-dev libpqxx-dev`
- **Missing submodules**: Run `git submodule update --init --recursive` first
- **Build directory conflicts**: Remove `build/` directory if builds fail: `rm -rf build`

## Runtime and Testing
- Start server: `./build/tiny_lobby` (after successful build)
- Server configuration: Edit `config.ini` for port, compression, and other settings
- Lua scripts: Located in `scripts/` directory (example: `scripts/echo/main.lua`)
- Test WebSocket connectivity: Use test suite in `test/` directory
- **Manual validation scenario**: After building, start server and run `cd test && npm run test` to validate WebSocket functionality

## Architecture and Development
- **Multi-threaded architecture**: WebSocket Thread, Game Thread, Database Thread, Authentication Thread
- **Key directories**: 
  - `src/`: C++ source code (websocket/, game/, database/, lua/, common/)
  - `scripts/`: Lua game logic scripts
  - `test/`: Node.js test suite with WebSocket client
  - `bash/`: Build and install scripts
- **Configuration files**: `config.ini` (server), `example_game_config.ini` (game settings)

## Platform-Specific Commands
- **Linux x64**: `./bash/install-linux-x64.sh` then `./bash/build_release-x64.sh`
- **Linux arm64**: `./bash/install-linux-arm64.sh` then `./bash/build_release-arm64.sh`  
- **macOS**: `./bash/install-macos-x64.sh` or `./bash/install-macos-arm64.sh` then corresponding build script
- **Debug builds**: Use `./bash/build_debug.sh` or `./bash/build_debug-x64.sh`

## Timing Expectations
- **Dependency installation**: 15+ minutes (NEVER CANCEL - set 30+ minute timeout)
- **Release build**: 2-5 minutes (set 15+ minute timeout)  
- **Test suite installation**: 30 seconds (set 2+ minute timeout)
- **Formatting check**: 10 seconds

## CI/CD Integration
- GitHub Actions workflow in `.github/workflows/build.yml`
- Lint step: runs `./bash/check_format.sh` and luau-analyze
- Multi-platform builds with vcpkg caching
- Docker builds for Linux variants
- Code signing for macOS releases

## Common Repo Commands Reference

### Repository root structure
```
.
├── README.md
├── BUILD_LOCALLY.md  
├── ARCHITECTURE.md
├── CMakeLists.txt
├── config.ini
├── example_game_config.ini
├── src/                    # C++ source code
├── scripts/                # Lua game scripts  
├── test/                   # Node.js test suite
├── bash/                   # Build scripts
├── external/vcpkg/         # Dependency management
└── build/                  # Build output (generated)
```

### Key vcpkg dependencies
```
boost-uuid, boost-container, boost-beast, inih, libpqxx, 
openssl, luau, zlib, efsw, libdeflate, uwebsockets
```