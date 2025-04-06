Write-Host "Installing RNP"
Set-Location external\rnp

# Create build directories
New-Item -ItemType Directory -Force -Path rnp-build
New-Item -ItemType Directory -Force -Path build-static
Set-Location rnp-build

# Run CMake configuration
cmake .. `
    -DCMAKE_INSTALL_PREFIX=../build-static `
    -DCMAKE_BUILD_TYPE=Release `
    -DBUILD_SHARED_LIBS=OFF `
    -DCMAKE_TOOLCHAIN_FILE=../../vcpkg/scripts/buildsystems/vcpkg.cmake `
    -DBUILD_TESTING=OFF `
    -DCMAKE_SYSTEM_PROCESSOR=x86_64 `
    -DVCPKG_TARGET_TRIPLET=x64-windows-static

# Compile with MSBuild
cmake --build . --config Release

# Install (to ../build-static)
cmake --install . --config Release

Write-Host "Installed RNP"
