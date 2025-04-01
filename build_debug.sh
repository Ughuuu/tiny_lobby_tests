rm -rf build
mkdir build
cd build
cmake -S .. -B . \
-DCMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake -G Ninja \
-DCMAKE_CXX_FLAGS="-DNDEBUG -DMOODYCAMEL_EXCEPTIONS_ENABLED" \
-DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
cd ..
codesign --entitlements ./debug.entitlements --sign "-" --force build/lobby_server
