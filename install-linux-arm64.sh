# Install deps
sudo apt-get update
sudo apt-get install -y build-essential zlib1g-dev
sudo apt-get install -y g++-aarch64-linux-gnu
sudo apt-get install -y g++-8 cmake libbz2-dev zlib1g-dev libjson-c-dev build-essential python-minimal
# Install packages
cd external/vcpkg
./bootstrap-vcpkg.sh
./vcpkg install boost-uuid --triplet arm64-linux
./vcpkg install inih --triplet arm64-linux
./vcpkg install libpqxx --triplet arm64-linux
./vcpkg install botan --triplet arm64-linux
./vcpkg install json-c --triplet arm64-linux
./vcpkg install openssl --triplet arm64-linux
./vcpkg install boost-container --triplet arm64-linux
./vcpkg install luau --triplet arm64-linux
./vcpkg install zlib --triplet arm64-linux
./vcpkg install bzip2 --triplet arm64-linux
./vcpkg install boost-beast --triplet arm64-linux
./vcpkg install efsw --triplet arm64-linux
./vcpkg integrate install
ls external/vcpkg/installed/arm64-linux/include
cd ../..
# Install rnp
./install-rnp-arm64.sh
