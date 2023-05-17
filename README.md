# Daedalus Engine
Game Engine. Personal project. Under early stage of development


## Builds

[![Windows-Master-CI](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Windows-Master-CI.yml/badge.svg?branch=master)](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Windows-Master-CI.yml)
[![Windows-Dev-CI](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Windows-Dev-CI.yml/badge.svg)](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Windows-Dev-CI.yml)
[![Linux-Master-CI](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Linux-Master-CI.yml/badge.svg?branch=master)](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Linux-Master-CI.yml)
[![Linux-Dev-CI](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Linux-Dev-CI.yml/badge.svg)](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Linux-Dev-CI.yml)


## How to Build

#### Windows:
- Install dependencies (only for first time building)<br>
 run InstallVCPKG.ps1 from WinScripts folder<br>
 run UpdatePackages.bat from WinScripts folder
<br>
<br>

- Config cmake project<br>
first time building: run CMakeConfigAndPackages.bat from WinScripts folder<br>
regular building: run CMakeConfig.bat from WinScripts folder
<br>
<br>

- Build sources:<br>
Build from IDE or CMakeBuildRelease.bat to build without IDE
<br>
<br>

#### Linux:
- Install dependencies

From project root:
````console
        cd LinuxScripts
        sudo sh InstallDependencies.sh
        sudo vcpkg install --triplet=x64-linux --overlay-triplets=ThirdParty\vcpkg_triplets\
````
<br>
<br>

- Config cmake project

From project root:
````console
cmake -B $PROJECT_ROOT/build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_TOOLCHAIN_FILE="$PROJECT_ROOT/ThirdParty/vcpkg/scripts/buildsystems/vcpkg.cmake"
````
<br>
<br>

- Build sources:

Build from IDE or `cmake --build $PROJECT_ROOT/build --config Release` from build folder
<br>
<br>
