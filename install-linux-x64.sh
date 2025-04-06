# Install deps
sudo apt-get update
sudo apt-get install -y build-essential zlib1g-dev
sudo apt-get install -y g++-8 cmake libbz2-dev zlib1g-dev libjson-c-dev build-essential python-minimal
# Install packages
cd external/vcpkg
./bootstrap-vcpkg.sh
./vcpkg install boost-uuid
./vcpkg install inih
./vcpkg install libpqxx
./vcpkg install botan
./vcpkg install json-c
./vcpkg install openssl
./vcpkg install "cpp-httplib[openssl]"
./vcpkg install boost-container
./vcpkg install luau
./vcpkg install zlib
./vcpkg integrate install
ls external/vcpkg/installed/x64-linux/include
cd ../..
# Install rnp
./install-rnp-x64.sh
