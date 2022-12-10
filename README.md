# Daedalus Engine
Game Engine


## Builds

[![Windows-Master-CI](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Windows-Master-CI.yml/badge.svg?branch=master)](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Windows-Master-CI.yml)
[![Windows-Dev-CI](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Windows-Dev-CI.yml/badge.svg)](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Windows-Dev-CI.yml)
[![Linux-Master-CI](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Linux-Master-CI.yml/badge.svg?branch=master)](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Linux-Master-CI.yml)
[![Linux-Dev-CI](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Linux-Dev-CI.yml/badge.svg)](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Linux-Dev-CI.yml)


## How to Build

#### Windows:
- Install dependencies

From project root:
````console
    cd ThirdParty/vcpkg
    ./bootstrap-vcpkg.bat
````
or run InstallVCPKG.ps1 from WinScripts folder
<br>
<br>

- Config cmake project

From project root:
````console
mkdir build
cd build
cmake ../ -DENABLE_TESTS=ON -DVCPKG_TARGET_TRIPLET=x64-windows
````
or run CMakeConfig.bat from WinScripts folder
<br>
<br>

- Build sources:

Build from IDE or `cmake ../ --build .` from build folder
<br>
<br>

#### Linux:
- Install dependencies

From project root:
````console
sudo apt update
sudo apt-get install libgl1-mesa-dev
sudo apt-get install mesa-utils
sudo apt install libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev

cd ThirdParty/vcpkg
./bootstrap-vcpkg.sh
````
or run InstallDependencies.sh from LinuxScripts folder
<br>
<br>

- Config cmake project

From project root:
````console
mkdir build
cd build
cmake ../ -DENABLE_TESTS=ON -DVCPKG_TARGET_TRIPLET=x64-linux
````
<br>
<br>

- Build sources:

Build from IDE or `cmake ../ --build .` from build folder
<br>
<br>
