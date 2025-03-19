# Install deps
sudo apt-get update
sudo apt-get install -y build-essential zlib1g-dev
# Build uWebSockets
cd external/uWebSockets
make
sudo make install
cd ../..
# Install packages
cd external/vcpkg
./bootstrap-vcpkg.sh
./vcpkg install boost-uuid
./vcpkg install inih
./vcpkg integrate install
cd ../..
# Install luajit
cd external/LuaJIT
make
sudo make install
cd ../..
