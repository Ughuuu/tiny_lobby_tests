# Build uWebSockets
cd external/uWebSockets
mingw32-make
mingw32-make install
cd ../..
# Install packages
cd external/vcpkg
bootstrap-vcpkg.bat
vcpkg install zlib
vcpkg install boost-uuid:x64-windows
vcpkg install readerwriterqueue:x64-windows
vcpkg install inih:x64-windows
vcpkg integrate install
cd ../..
# Install luajit
cd external/luajit
mingw32-make
mingw32-make install
cd ../..
