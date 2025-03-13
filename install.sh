cd external/uWebSockets
make
sudo make install
cd ../..
cd external/uuid_v4
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX="/usr/local" ..
sudo cmake --install .
