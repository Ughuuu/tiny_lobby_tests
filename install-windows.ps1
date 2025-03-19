Write-Output "Installing dependencies for Windows"
# Set WITH_ZLIB to 0
$env:WITH_ZLIB = '0'

Write-Output "Making and installing uWebSockets"
# Build uWebSockets
Set-Location -Path "external\uWebSockets"
mingw32-make
mingw32-make install
Set-Location -Path "..\.."

Write-Output "Downloading boost-uuid and inih using vcpkg"
# Install packages using vcpkg
Set-Location -Path "external\vcpkg"
.\bootstrap-vcpkg.bat
.\vcpkg install boost-uuid:x64-windows
.\vcpkg install inih:x64-windows
.\vcpkg integrate install
Set-Location -Path "..\.."

Write-Output "Making and installing luajit"
# Install luajit
Set-Location -Path "external\luajit"
mingw32-make
mingw32-make install
Set-Location -Path "..\.."
