echo "Installing RNP"
cd external/rnp
mkdir rnp-build
mkdir build-static
cd rnp-build

# Run CMake
cmake -DCMAKE_INSTALL_PREFIX=../build-static \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
    -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
    -DCMAKE_TOOLCHAIN_FILE=../../vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=arm64-linux \
    -DBUILD_TESTING=OFF \
    -DCMAKE_FIND_LIBRARY_SUFFIXES=".a" \
    ..

# Compile
make

# Install
make install
echo "Installed RNP"
