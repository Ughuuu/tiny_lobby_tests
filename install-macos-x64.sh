export VCPKG_FORCE_SYSTEM_BINARIES=1
export VCPKG_DEFAULT_TRIPLET=x64-osx
export CMAKE_OSX_ARCHITECTURES="x86_64"
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
./vcpkg install cpp-httplib --triplet x64-osx
./vcpkg install boost-container --triplet x64-osx
./vcpkg list
./vcpkg integrate install
cd ../..
# Install luajit
cd external/LuaJIT
make MACOSX_DEPLOYMENT_TARGET=10.15 CC="gcc -m64 -arch x86_64"
sudo make install
cd ../..
# Install rnp
./install-rnp.sh
