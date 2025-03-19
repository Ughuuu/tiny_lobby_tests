#rm -rf build
mkdir build
cd build
cmake -S .. -B . \
-DCMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake \
-DCMAKE_BUILD_TYPE=RelWithDebInfo \
-DVCPKG_TARGET_TRIPLET=arm64-osx \
-DVCPKG_TARGET_TRIPLET=x64-osx \
-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build . --config RelWithDebInfo
cd ..
