cd ../
cd ThirdParty/vcpkg
./bootstrap-vcpkg.sh
./vcpkg install glfw3 --triplet=x64-linux
./vcpkg install gtest --triplet=x64-linux
./vcpkg install glad[gl-api-latest] --triplet=x64-linux --recurse