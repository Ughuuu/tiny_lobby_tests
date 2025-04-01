# Install deps
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
./vcpkg install "cpp-httplib[openssl]" --triplet arm64-osx
./vcpkg install boost-container --triplet arm64-osx
./vcpkg list
./vcpkg integrate install
cd ../..
# Install luajit
cd external/LuaJIT
make MACOSX_DEPLOYMENT_TARGET=10.15
sudo make install
cd ../..
# Install rnp
./install-rnp.sh
