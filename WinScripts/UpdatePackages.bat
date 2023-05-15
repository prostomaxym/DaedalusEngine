@echo off

cd ../

.\ThirdParty\vcpkg\vcpkg.exe install --triplet=x64-windows --overlay-triplets=ThirdParty\vcpkg_triplets\

PAUSE