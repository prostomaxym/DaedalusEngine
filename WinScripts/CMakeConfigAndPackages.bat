@echo off

cd ..

if not exist build\ (
  mkdir build
)

cd build

cmake .. -DENABLE_TESTS=ON -DCMAKE_TOOLCHAIN_FILE=ThirdParty/vcpkg/scripts/buildsystems/vcpkg.cmake

PAUSE