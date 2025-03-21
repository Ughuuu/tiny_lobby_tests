export VCPKG_FORCE_SYSTEM_BINARIES=1
export VCPKG_DEFAULT_TRIPLET=x64-osx
export CMAKE_OSX_ARCHITECTURES="x86_64"

# Install deps
brew install zlib autoconf automake libtool
# Install packages
cd external/vcpkg
./bootstrap-vcpkg.sh
./vcpkg install boost-uuid --triplet x64-osx
./vcpkg install inih --triplet x64-osx
./vcpkg install luajit --triplet x64-osx
./vcpkg install libpqxx --triplet x64-osx
./vcpkg list
./vcpkg integrate install
cd ../..
