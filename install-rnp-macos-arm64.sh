echo "Installing RNP"
cd external/rnp
mkdir rnp-build
mkdir build-static
cd rnp-build

# Run CMake
cmake -DCMAKE_INSTALL_PREFIX=../build-static \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_TOOLCHAIN_FILE=../../vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DBUILD_TESTING=OFF \
    -DCMAKE_FIND_LIBRARY_SUFFIXES=".a" \
    ..

# Compile
make

# Install
make install
echo "Installed RNP"
