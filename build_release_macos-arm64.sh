#rm -rf build
mkdir build
cd build
cmake -S .. -B . \
-DCMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake \
-DCMAKE_BUILD_TYPE=RelWithDebInfo \
-DVCPKG_TARGET_TRIPLET=arm64-osx \
-DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O3 -march=native -flto -DNDEBUG" \
-DCMAKE_OSX_ARCHITECTURES="arm64"
#-DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g -O1" \
#-DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
cmake --build . --config RelWithDebInfo
cd ..
