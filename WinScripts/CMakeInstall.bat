@echo off

cd ../
cd build

cmake --install . --prefix ../runtime/install --config Release

PAUSE