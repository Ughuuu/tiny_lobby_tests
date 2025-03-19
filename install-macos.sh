# Install deps
brew install zlib
# Build uWebSockets
cd external/uWebSockets
make
sudo make install
cd ../..
# Install packages
cd external/vcpkg
./bootstrap-vcpkg.sh
./vcpkg install boost-uuid --triplet arm64-osx
./vcpkg install boost-uuid --triplet x64-osx
./vcpkg install inih --triplet arm64-osx
./vcpkg install inih --triplet x64-osx
./vcpkg list
./vcpkg integrate install
cd ../..
# Install luajit
cd external/LuaJIT
make MACOSX_DEPLOYMENT_TARGET=10.15 ARCH=arm64
sudo make install
cd ../..
