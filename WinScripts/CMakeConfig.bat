@echo off

cd ..

if not exist build\ (
  mkdir build
)

cd build

cmake .. -DENABLE_TESTS=ON

PAUSE