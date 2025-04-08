Write-Output "Building the project in release mode"

# Get the current working directory
$currentDir = Get-Location

# Set the full path to the toolchain file
$toolchainFile = Join-Path $currentDir "external/vcpkg/scripts/buildsystems/vcpkg.cmake"

# Create the build directory if it doesn't exist
New-Item -ItemType Directory -Path "build" -Force | Out-Null

# Change to the build directory
Set-Location -Path "build"

Write-Output "Running cmake"
# Run cmake with the specified arguments
$currDir = Get-Location
Write-Output "Current directory: $currDir"
Get-ChildItem ../external/vcpkg/scripts/buildsystems
cmake -S .. -B . `
    -DCMAKE_TOOLCHAIN_FILE="$toolchainFile" `
    -DVCPKG_TARGET_TRIPLET="x64-windows-static" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_ASM_MASM_FLAGS="" 

# Build the project
cmake --build . --config Release

# Go back to the previous directory
Set-Location -Path $currentDir
