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
        sh InstallDependencies.sh
        sh UpdatePackages.sh
````
<br>
<br>

- Config cmake project

From LinuxScripts folder:
````console
        sh CMakeConfig.sh      -for MakeFile
                or
        sh CMakeConfigCB.sh    -for CodeBlocks IDE
                or
        sh CMakeConfigCL.sh    -for CodeLite IDE
````
<br>
<br>

- Build sources:

Build from IDE or `cmake --build $PROJECT_ROOT/build --config Release` from build folder
<br>
<br>


For demo level runtime assets should be downloaded separately and put under $PROJECT_ROOT/runtime/assets 
