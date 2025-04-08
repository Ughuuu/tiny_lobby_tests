Write-Host "Installing RNP"
Set-Location external\rnp

# Create build directories
New-Item -ItemType Directory -Force -Path rnp-build
New-Item -ItemType Directory -Force -Path build-static
Set-Location rnp-build

$VcpkgRoot = Resolve-Path ../../vcpkg
$VcpkgTriplet = "x64-windows-static"
$VcpkgInstalledPath = Join-Path $VcpkgRoot "installed\$VcpkgTriplet"

$InstallPath = Resolve-Path ../build-static

# Run CMake configuration
cmake .. `
    -DCMAKE_INSTALL_PREFIX="$InstallPath" `
    -DCMAKE_BUILD_TYPE=Release `
    -DBUILD_SHARED_LIBS=OFF `
    -DBOTAN_STATIC=ON `
    -DBOTAN_LIBRARY="$VcpkgInstalledPath/lib/botan.lib" `
    -DBOTAN_INCLUDE_DIR="$VcpkgInstalledPath/include" `
    -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot/scripts/buildsystems/vcpkg.cmake" `
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
    -DBUILD_TESTING=OFF `
    -DCMAKE_PREFIX_PATH="$VcpkgInstalledPath" `
    -DCMAKE_SYSTEM_PROCESSOR=x86_64 `
    -DVCPKG_TARGET_TRIPLET=x64-windows-static `
    -DCMAKE_FIND_LIBRARY_SUFFIXES=".lib" `
    -DCMAKE_FIND_LIBRARY_PREFIXES="lib;"

# Compile with MSBuild
cmake --build . --config Release

# Install (to ../build-static)
cmake --install . --config Release

Write-Host "Installed RNP"
