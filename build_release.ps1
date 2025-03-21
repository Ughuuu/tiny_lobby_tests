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
    -DCMAKE_BUILD_TYPE=RelWithDebInfo `
    -DCMAKE_ASM_MASM_FLAGS="" # `
    #-DCMAKE_CXX_FLAGS_RELWITHDEBINFO="/O2 /GL /DNDEBUG"

# Build the project
cmake --build . --config RelWithDebInfo

# Go back to the previous directory
Set-Location -Path $currentDir
