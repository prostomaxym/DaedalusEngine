# Daedalus Engine
Cross-platform (Windows + Linux) C++17 game engine. Personal project. Under early stage of development

## Builds

[![Windows-Master-CI](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Windows-Master-CI.yml/badge.svg?branch=master)](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Windows-Master-CI.yml)
[![Windows-Dev-CI](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Windows-Dev-CI.yml/badge.svg)](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Windows-Dev-CI.yml)
[![Linux-Master-CI](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Linux-Master-CI.yml/badge.svg?branch=master)](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Linux-Master-CI.yml)
[![Linux-Dev-CI](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Linux-Dev-CI.yml/badge.svg)](https://github.com/prostomaxym/daedalus_engine/actions/workflows/Linux-Dev-CI.yml)

## Requirements
- CMake 3.21+ (required by `CMakePresets.json`)
- Windows: Visual Studio with the C++ workload (MSVC), PowerShell
- Linux: GCC or Clang, Make/Ninja
- Git (vcpkg is a submodule)

Dependencies (glfw3, glad, imgui, glm, assimp, entt, spdlog, bullet3, gtest, ...) are installed by vcpkg in manifest mode (`vcpkg.json`).

## Build configurations

| Configuration | Description |
|---|---|
| `Debug` | Unoptimized, with asserts and dev tools |
| `Release` | Clean production build |
| `ReleaseDevTools` | Release optimizations with the `DL_DEV_TOOLS` define (extra dev tools) |

On Windows (Visual Studio generator) all three are available in the generated `.sln`.
On Linux (single-config generators) pick one with `-DCMAKE_BUILD_TYPE=<config>`; the default is `Release`.

## How to Build

Clone with submodules:
```console
git clone --recursive https://github.com/prostomaxym/daedalus_engine
```
or run `git submodule update --init` in an existing clone.

#### Windows
1. Install dependencies (first time only), from the `WinScripts` folder:
   - `InstallVCPKG.ps1` - bootstraps vcpkg
   - `UpdatePackages.bat` - installs vcpkg packages
2. Configure the project (generates `build/*.sln`):
   - `WinScripts/CMakeConfig.bat` (same as `cmake --preset windows`)
   - `WinScripts/CMakeCleanConfig.bat` deletes `build` and configures from scratch
3. Build:
   - Open the generated `.sln` in `build/` and select a configuration, or
   - `WinScripts/CMakeBuildRelease.bat`, or from the root:
```console
cmake --build --preset windows-release
cmake --build --preset windows-debug
cmake --build --preset windows-release-devtools
```

The preset enables unit tests (`ENABLE_TESTS=ON`). Output goes to `runtime/bin/windows-x64/<Configuration>/`.

#### Linux
1. Install dependencies, from the `LinuxScripts` folder:
```console
sh InstallDependencies.sh
sh UpdatePackages.sh
```
2. Configure, from the `LinuxScripts` folder:
```console
sh CMakeConfig.sh         # Makefiles
sh CMakeConfigCB.sh       # CodeBlocks IDE
sh CMakeConfigCL.sh       # CodeLite IDE (Release)
sh CMakeConfigCLDebug.sh  # CodeLite IDE (Debug)
```
3. Build:
```console
sh CMakeBuildRelease.sh
```
or `cmake --build --preset linux-release` from the project root.

## Tests
Unit tests (gtest) are built with `ENABLE_TESTS=ON`, which the presets set. From the project root:
```console
ctest --test-dir build -C Release --output-on-failure
```
or run `dlUnitTests` from `runtime/bin/<platform>-x64/`.

## Install and package
- Windows: `WinScripts/CMakeInstallRelease.bat` or `WinScripts/CMakeInstallReleaseDevTools.bat` (installs the selected built configuration to `runtime/install`), `WinScripts/CPackInstallerRelease.bat` or `WinScripts/CPackInstallerReleaseDevTools.bat` (creates the corresponding installer)
- Linux: `LinuxScripts/CMakeInstall.sh`, `LinuxScripts/CPackInstaller.sh`

Windows installation and packaging support `Release` (without developer tools) and `ReleaseDevTools` (with developer tools). Build the selected configuration before installing or packaging. Both install helpers use the same destination; both packaging helpers write the configured installer filename to `runtime`, replacing the previous installer. Linux helpers are unchanged.

The Windows NSIS uninstaller automatically removes installation-local `logs` and `shader/cache` folders. It asks **Remove settings and user data?**, defaulting to **No**. Choosing **Yes** recursively deletes only the installation-local `config` folder; choosing **No** preserves the entire folder, including packaged settings files. Silent uninstallation preserves configs as well. If settings cannot be safely preserved, uninstallation stops instead of deleting them.

Cleanup does not follow custom paths from `path.ini` or recursively remove asset folders. Other package-owned files are removed normally; manually added assets and external folders are not additional cleanup targets. Linux packaging is unchanged.

## Assets
For the demo level, runtime assets should be downloaded separately and put under `$PROJECT_ROOT/runtime/assets`.
