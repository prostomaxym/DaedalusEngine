@echo off

cd ..

if not exist build\ (
  mkdir build
)

cd build

cmake .. -G "Visual Studio 18 2026" -A x64 -DENABLE_TESTS=ON -DCMAKE_TOOLCHAIN_FILE="ThirdParty/vcpkg/scripts/buildsystems/vcpkg.cmake"

PAUSE