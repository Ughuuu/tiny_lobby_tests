# Install deps
brew install zlib autoconf automake libtool
arch -x86_64 brew install zlib autoconf automake libtool
# Install packages
cd external/vcpkg
./bootstrap-vcpkg.sh
./vcpkg install boost-uuid --triplet x64-osx
./vcpkg install inih --triplet x64-osx
./vcpkg install libpqxx --triplet x64-osx
./vcpkg install botan --triplet x64-osx
./vcpkg install json-c --triplet x64-osx
./vcpkg install openssl --triplet x64-osx
./vcpkg install "cpp-httplib[openssl]" --triplet x64-osx
./vcpkg install boost-container --triplet x64-osx
./vcpkg install luau --triplet x64-osx
./vcpkg install zlib --triplet x64-osx
./vcpkg list
./vcpkg integrate install
cd ../..
# Install rnp
./install-rnp-macos-x64.sh
