#rm -rf build
mkdir build
cd build
cmake -S .. -B . \
-DCMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake \
-DVCPKG_TARGET_TRIPLET="x64-linux" \
-DCMAKE_BUILD_TYPE=Debug \
-DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g -O1" \
cmake --build . --config Debug
cd ..
