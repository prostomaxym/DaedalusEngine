#!/bin/bash

cd ..

if [ ! -d "build" ]; then
  mkdir build
fi

cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_TOOLCHAIN_FILE="ThirdParty/vcpkg/scripts/buildsystems/vcpkg.cmake" -G"CodeBlocks - Unix Makefiles"
