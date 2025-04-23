#rm -rf build
mkdir build
cd build
cmake -S .. -B . \
-DCMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake \
-DVCPKG_TARGET_TRIPLET="arm64-linux" \
-DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
-DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
-DCMAKE_SYSTEM_NAME=Linux \
-DCMAKE_SYSTEM_PROCESSOR=aarch64 \
-DCMAKE_BUILD_TYPE=ReleaseWithDebInfo \
-DCMAKE_CXX_FLAGS_RELEASE_WITH_DEBINFO="-O3 -flto -DNDEBUG"

cmake --build . --config ReleaseWithDebInfo
cd ..
