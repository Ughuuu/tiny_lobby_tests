# Build uWebSockets
cd external/uWebSockets
make
sudo make install
cd ../..
# Install packages
cd external/vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
./vcpkg install boost-uuid
./vcpkg install simdjson
./vcpkg install readerwriterqueue
./vcpkg install inih
cd ../..
# Install luajit
cd external/luajit
make MACOSX_DEPLOYMENT_TARGET=10.15
sudo make install
