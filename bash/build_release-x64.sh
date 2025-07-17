#rm -rf build
mkdir build
cd build
cmake -S .. -B . \
-DCMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake \
-DVCPKG_TARGET_TRIPLET="x64-linux" \
-DCMAKE_BUILD_TYPE=Release \
-DCMAKE_CXX_FLAGS_RELEASE="-O3 -flto -DNDEBUG"
cmake --build . --config Release
cd ..
