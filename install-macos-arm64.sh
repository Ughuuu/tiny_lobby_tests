# Install deps
brew install zlib autoconf automake libtool
# Install packages
cd external/vcpkg
./bootstrap-vcpkg.sh
./vcpkg install boost-uuid --triplet arm64-osx
./vcpkg install inih --triplet arm64-osx
./vcpkg install luajit --triplet arm64-osx
./vcpkg install libpqxx --triplet arm64-osx
./vcpkg list
./vcpkg integrate install
cd ../..
