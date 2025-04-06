#rm -rf build
mkdir build
cd build
cmake -S .. -B . \
-DCMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake \
-DCMAKE_BUILD_TYPE=RelWithDebInfo \
-DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O3 -flto -DNDEBUG" \
-DCMAKE_OSX_ARCHITECTURES="x86_64"
cmake --build . --config RelWithDebInfo
cd ..
