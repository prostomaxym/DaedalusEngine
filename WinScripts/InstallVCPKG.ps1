cd ../
cd ThirdParty/vcpkg
./bootstrap-vcpkg.bat
./vcpkg --triplet=x64-windows install glfw3
./vcpkg --triplet=x64-windows install gtest