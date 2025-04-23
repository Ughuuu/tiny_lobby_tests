# Install deps
brew install zlib autoconf automake libtool
arch -arm64 brew install zlib autoconf automake libtool
# Install packages
cd external/vcpkg
./bootstrap-vcpkg.sh
./vcpkg install boost-uuid --triplet arm64-osx
./vcpkg install inih --triplet arm64-osx
./vcpkg install libpqxx --triplet arm64-osx
./vcpkg install botan --triplet arm64-osx
./vcpkg install json-c --triplet arm64-osx
./vcpkg install openssl --triplet arm64-osx
./vcpkg install boost-container --triplet arm64-osx
./vcpkg install luau --triplet arm64-osx
./vcpkg install zlib --triplet arm64-osx
./vcpkg install boost-beast --triplet arm64-osx
./vcpkg list
./vcpkg integrate install
cd ../..
# Install rnp
./install-rnp-macos-arm64.sh
