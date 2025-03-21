Write-Output "Downloading deps using vcpkg"
# Install packages using vcpkg
Set-Location -Path "external\vcpkg"
.\bootstrap-vcpkg.bat
.\vcpkg install boost-uuid:x64-windows
.\vcpkg install inih:x64-windows
.\vcpkg install luajit:x64-windows
.\vcpkg install libpqxx:x64-windows
.\vcpkg integrate install
Set-Location -Path "..\.."
