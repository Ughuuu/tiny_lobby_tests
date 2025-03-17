#rm -rf build
mkdir build
cd build
cmake -S .. -B . \
-DCMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake -G ninja \
-DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
