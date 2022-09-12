cd ../
cd ThirdParty/vcpkg
./bootstrap-vcpkg.bat
./vcpkg --triplet=x64-windows install glfw3
./vcpkg --triplet=x64-windows install spdlog
./vcpkg --triplet=x64-windows install fmt --v 8.1.1
./vcpkg --triplet=x64-windows install gtest
pause