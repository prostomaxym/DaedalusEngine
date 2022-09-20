cd ../
cd ThirdParty/vcpkg
./bootstrap-vcpkg.bat
./vcpkg install glfw3 --triplet=x64-windows --x-install-root=C:\Users\ershi\source\repos\prostomaxym\Daedalus\ThirdParty\vcpkg\installed
./vcpkg install gtest --triplet=x64-windows --x-install-root=C:\Users\ershi\source\repos\prostomaxym\Daedalus\ThirdParty\vcpkg\installed
./vcpkg install glad[gl-api-latest] --triplet=x64-windows --recurse --x-install-root=$(get-location)\..\ThirdParty\vcpkg\installed