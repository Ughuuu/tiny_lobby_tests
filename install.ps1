Write-Output "Downloading deps using vcpkg"
# Install packages using vcpkg
Set-Location -Path "external\vcpkg"
.\bootstrap-vcpkg.bat
.\vcpkg install boost-uuid --disable-metrics --triplet x64-windows-static
.\vcpkg install inih --disable-metrics --triplet x64-windows-static
.\vcpkg install libpqxx --disable-metrics --triplet x64-windows-static
.\vcpkg install botan --disable-metrics --triplet x64-windows-static
.\vcpkg install json-c --disable-metrics --triplet x64-windows-static
.\vcpkg install openssl --disable-metrics --triplet x64-windows-static
.\vcpkg install "cpp-httplib[openssl]" --disable-metrics --triplet x64-windows-static
.\vcpkg install boost-container --disable-metrics --triplet x64-windows-static
.\vcpkg install luau --disable-metrics --triplet x64-windows-static
.\vcpkg install zlib --disable-metrics --triplet x64-windows-static
.\vcpkg install bzip2 --disable-metrics --triplet x64-windows-static
.\vcpkg integrate install
Set-Location -Path "..\.."

Write-Output "Installing RNP"
# Install RNP
.\install-rnp.ps1
