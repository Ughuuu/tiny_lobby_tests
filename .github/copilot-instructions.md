# Tiny Lobby Server
Tiny Lobby is a C++ multiplayer lobby server with WebSocket communication and Lua scripting support. It uses CMake with vcpkg dependency management and supports cross-platform builds.

Always reference these instructions first and fallback to search or bash commands only when you encounter unexpected information that does not match the info here.

## Working Effectively
- **CRITICAL: KNOWN BUILD ISSUE**: vcpkg installation of libpqxx frequently fails due to PostgreSQL FTP server connectivity issues (`ftp.postgresql.org` DNS/timeout failures). This is a known environmental limitation, not a code issue.
- Bootstrap dependencies and attempt build:
  - `git submodule update --init --recursive` -- initializes vcpkg submodule
  - `sudo apt-get update && sudo apt-get install -y build-essential zlib1g-dev g++-8 cmake libbz2-dev libjson-c-dev python-minimal`
  - `./bash/install-linux-x64.sh` -- takes 15+ minutes. NEVER CANCEL. Set timeout to 30+ minutes.
  - **EXPECT POTENTIAL FAILURE**: If libpqxx installation fails with PostgreSQL download errors, this is a known network issue
- **Build alternatives when vcpkg fails**:
  - Install system PostgreSQL packages: `sudo apt-get install -y libpq-dev libpqxx-dev`
  - Note: vcpkg CMake toolchain may still prevent build completion with system packages
  - In network-restricted environments, the full build may not be achievable
- Test suite (Node.js based):
  - `cd test && npm install` -- takes 30 seconds. Set timeout to 2+ minutes.
  - `npm run unit` -- runs unit tests (requires running server on port 8080)
  - `npm run test` -- runs performance tests against server

## Validation and Development Tools
- **NEVER CANCEL builds or dependency installations** - vcpkg dependency installation takes 15+ minutes, potential build failures are environmental
- **Code quality tools that WORK**:
  - `./bash/format.sh` -- applies clang-format to all C++ files
  - `./bash/check_format.sh` -- validates code formatting (required for CI)
  - Download and run Lua analysis: `wget https://github.com/luau-lang/luau/releases/download/0.667/luau-ubuntu.zip && unzip luau-ubuntu.zip && chmod +x luau-analyze && ./luau-analyze scripts/**/*.lua`
- **Test environment setup**:
  - `cd test && npm install` -- installs WebSocket test client and testing frameworks
  - Tests require running server: Unit tests expect server on `localhost:8080`
- **Manual validation scenarios**: 
  - **Code formatting**: Run `./bash/check_format.sh` - should output "All files are correctly formatted!"
  - **Lua validation**: Run `./luau-analyze scripts/**/*.lua` - should complete without errors
  - **Test dependencies**: Run `cd test && npm install` - should complete in ~30 seconds
  - **Build validation**: If vcpkg completes successfully, `./bash/build_release-x64.sh` should produce `build/tiny_lobby` binary

## Common Build Issues and Network Dependencies
- **libpqxx PostgreSQL download failure**: This is the most common issue in restricted network environments
  - Symptom: `curl: (6) Could not resolve host: ftp.postgresql.org` or timeout errors
  - Root cause: Network restrictions or DNS issues with PostgreSQL FTP servers
  - Impact: Prevents complete vcpkg dependency installation
  - **No complete workaround exists** when using vcpkg CMake toolchain
- **Build environment requirements**: 
  - Unrestricted network access to vcpkg package sources
  - clang 16.0.0+ or gcc 13.3.0+, cmake 3.24+
  - 15+ minutes for initial dependency installation
- **Successful build indicators**:
  - vcpkg should install all packages including libpqxx without network errors
  - `external/vcpkg/installed/x64-linux/` should contain include/, lib/, share/ directories
  - Build should complete in 2-5 minutes once dependencies are ready

## Common Build Issues
- **libpqxx network failure**: Most common issue in network-restricted environments
- **Missing submodules**: Always run `git submodule update --init --recursive` first
- **Build directory conflicts**: Remove `build/` directory if builds fail: `rm -rf build`
- **vcpkg cache issues**: Clear vcpkg cache if needed: `rm -rf ~/.cache/vcpkg`

## Runtime and Testing  
- **Server startup**: `./build/tiny_lobby` (requires successful build)
- **Configuration**: Edit `config.ini` - server runs on port 8080, database disabled by default
- **Lua scripts**: Located in `scripts/` directory (example: `scripts/echo/main.lua`)
- **WebSocket testing**: Use Node.js test suite in `test/` directory
- **Docker**: `docker build -t tiny_lobby .` (requires pre-built binary in `build/tiny_lobby`)

## Measured Timing Expectations (NEVER CANCEL)
- **vcpkg bootstrap and dependency installation**: 15+ minutes (measured: 11.5 minutes in successful environments)
- **Release build**: 2-5 minutes (when dependencies are ready)
- **Test dependencies (npm install)**: 30 seconds  
- **Code formatting check**: <10 seconds
- **Lua analysis**: <30 seconds

## CI/CD Integration
- **GitHub Actions**: `.github/workflows/build.yml` handles multi-platform builds
- **Required checks**: Code formatting (`check_format.sh`) and Lua analysis must pass
- **Platform support**: Linux (x64/arm64), macOS (x64/arm64), Windows (x64)
- **Artifacts**: Produces signed binaries and Docker images on successful builds
- **Known CI difference**: CI environment has full network access, unlike some development environments

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

### Validated Working Components (Tested)
- ✅ **Submodule initialization**: `git submodule update --init --recursive` 
- ✅ **System packages**: Ubuntu apt packages install correctly
- ✅ **vcpkg bootstrap**: `./external/vcpkg/bootstrap-vcpkg.sh` works
- ✅ **Most vcpkg packages**: All packages except libpqxx install successfully  
- ✅ **Code formatting**: `./bash/format.sh` and `./bash/check_format.sh` work correctly
- ✅ **Lua analysis**: luau-analyze validates scripts successfully
- ✅ **Test environment**: `cd test && npm install` completes in ~30 seconds
- ✅ **Development tools**: clang, cmake, node.js all function correctly

### Known Issues (Environmental)
- ❌ **libpqxx vcpkg installation**: Fails due to ftp.postgresql.org network restrictions
- ❌ **Complete C++ build**: Cannot complete without all vcpkg dependencies  
- ❌ **Runtime testing**: Requires successful binary build
- ❌ **Integration tests**: Need running server for WebSocket client testing

**Note**: These issues are environmental (network restrictions) rather than code issues. In unrestricted network environments, the CI pipeline demonstrates that full builds work correctly.