cd ../
cd ThirdParty/vcpkg
./bootstrap-vcpkg.sh
./vcpkg --triplet=x64-linux install glfw3
./vcpkg --triplet=x64-linux install gtest
./cpkg --triplet=x64-linux install glad