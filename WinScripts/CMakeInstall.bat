@echo off

cd ../
cd build

cmake --install . --prefix ../install --config Release

PAUSE