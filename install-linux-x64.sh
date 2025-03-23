# Install deps
sudo apt-get update
sudo apt-get install -y build-essential zlib1g-dev
# Install packages
cd external/vcpkg
./bootstrap-vcpkg.sh
./vcpkg install boost-uuid
./vcpkg install inih
./vcpkg install libpqxx
./vcpkg install gpgme
./vcpkg install cpp-httplib
./vcpkg install boost-container
./vcpkg integrate install
ls external/vcpkg/installed/x64-linux/include
cd ../..
# Install luajit
cd external/LuaJIT
make
sudo make install
cd ../..
