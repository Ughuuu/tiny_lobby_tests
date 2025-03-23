#rm -rf build
mkdir build
cd build
cmake -S .. -B . \
-DCMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake \
-DCMAKE_BUILD_TYPE=RelWithDebInfo \
-DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O3 -march=arm64 -flto -DNDEBUG" \
-DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
-DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++

cmake --build . --config RelWithDebInfo
cd ..
