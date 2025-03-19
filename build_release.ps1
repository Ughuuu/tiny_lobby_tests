Write-Host "Building the project in release mode"
# Remove the build directory if it exists
Remove-Item -Recurse -Force -Path "build"

# Create the build directory
New-Item -ItemType Directory -Path "build"

# Change to the build directory
Set-Location -Path "build"

Write-Host "Running cmake"
# Run cmake with the specified arguments
cmake -S .. -B . `
    -G "Visual Studio 17 2022" `
    -DCMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake `
    -DCMAKE_BUILD_TYPE=RelWithDebInfo `
    -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="/O2 /GL /DNDEBUG"

cmake --build . --config RelWithDebInfo
# Go back to the previous directory
Set-Location -Path ".."
