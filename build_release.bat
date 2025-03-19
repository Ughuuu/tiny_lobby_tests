#rm -rf build
mkdir build
cd build
cmake -S .. -B . ^
    -G "Visual Studio 17 2022" ^
    -DCMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake ^
    -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
    -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="/O2 /GL /DNDEBUG"
cd ..
