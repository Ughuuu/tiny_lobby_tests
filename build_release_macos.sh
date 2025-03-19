#rm -rf build
mkdir build
cd build
cmake -S .. -B . \
-DCMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake \
-DCMAKE_BUILD_TYPE=RelWithDebInfo \
-DVCPKG_TARGET_TRIPLET=x64-osx \
-DVCPKG_TARGET_TRIPLET=arm64-osx \
-DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
#-DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O3 -march=native -flto -DNDEBUG"
cmake --build . --config RelWithDebInfo
cd ..
