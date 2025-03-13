cd external/uWebSockets
make
sudo make install
cd ../..
cd external/uuid_v4
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX="/usr/local" ..
sudo cmake --install .
cd external/vcpkg
./bootstrap-vcpkg.sh
cd ../..
./external/vcpkg/vcpkg install boost-uuid
./external/vcpkg/vcpkg install simdjson
./external/vcpkg/vcpkg integrate install
.\vcpkg.exe install simdjson:x64-windows
