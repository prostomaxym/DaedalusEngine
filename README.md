# Daedalus Engine
Cross-platform (Windows + Linux + Android) C++20 game engine. Personal project. Under early stage of development

## Builds

[![Windows-Dev-CI](https://github.com/prostomaxym/DaedalusEngine/actions/workflows/Windows-Dev-CI.yml/badge.svg)](https://github.com/prostomaxym/DaedalusEngine/actions/workflows/Windows-Dev-CI.yml)
[![Linux-Dev-CI](https://github.com/prostomaxym/DaedalusEngine/actions/workflows/Linux-Dev-CI.yml/badge.svg)](https://github.com/prostomaxym/DaedalusEngine/actions/workflows/Linux-Dev-CI.yml)
[![Android-Dev-CI](https://github.com/prostomaxym/DaedalusEngine/actions/workflows/Android-Dev-CI.yml/badge.svg)](https://github.com/prostomaxym/DaedalusEngine/actions/workflows/Android-Dev-CI.yml)

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
git clone --recursive https://github.com/prostomaxym/DaedalusEngine
```
or run `git submodule update --init` in an existing clone.

#### Windows
1. Bootstrap vcpkg (first time only): `./WinScripts/InstallVCPKG.ps1`.
2. Configure the project (generates the Visual Studio solution under `build` and installs manifest dependencies):
   - `WinScripts/WindowsConfig.bat` (same as `cmake --preset windows`)
   - `WinScripts/WindowsCleanConfig.bat` deletes build folders and configures from scratch while preserving installed dependencies
3. Build:
   - Open the generated `.sln` in `build/` and select a configuration, or
	  - `WinScripts/WindowsBuildRelease.bat`, or from the root:
```console
cmake --build --preset windows-release
cmake --build --preset windows-debug
cmake --build --preset windows-release-devtools
```

The preset enables unit tests (`ENABLE_TESTS=ON`). Output goes to `runtime/bin/windows-x64/<Configuration>/`.

#### Linux
1. Install dependencies, from the `LinuxScripts` folder:
```console
bash LinuxInstallDependencies.sh
```
2. Configure, from the `LinuxScripts` folder:
```console
bash LinuxConfig.sh         # Default generator (Release)
bash LinuxConfigCB.sh       # CodeBlocks IDE
bash LinuxConfigCL.sh       # CodeLite IDE (Release)
bash LinuxConfigCLDebug.sh  # CodeLite IDE (Debug)
```
3. Build:
```console
bash LinuxBuildRelease.sh
```
or `cmake --build --preset linux-release` from the project root. Configuration installs vcpkg manifest dependencies automatically. See [LinuxScripts/README.md](LinuxScripts/README.md) for all helpers, including Debug and ReleaseDevTools workflows.

Desktop and Android dependencies are installed under `ThirdParty/vcpkg_installed`; all custom triplets live under `ThirdParty/vcpkg_triplets`. Preserve both directories when cleaning generated build artifacts. The vcpkg submodule remains at `ThirdParty/vcpkg`.

## Tests
Unit tests (gtest) are built with `ENABLE_TESTS=ON`, which the presets set. From the project root:
```console
ctest --test-dir build -C Release --output-on-failure
```
or run `dlUnitTests` from `runtime/bin/<platform>-x64/`.

## Install and package
- Windows: `WinScripts/WindowsInstallRelease.bat` or `WinScripts/WindowsInstallReleaseDevTools.bat` (installs the selected built configuration to `runtime/install`), `WinScripts/WindowsPackRelease.bat` or `WinScripts/WindowsPackReleaseDevTools.bat` (creates the corresponding installer)
- Linux: `LinuxScripts/LinuxInstallRelease.sh` or `LinuxScripts/LinuxInstallReleaseDevTools.sh` (installs the selected built configuration to `runtime/install`), `LinuxScripts/LinuxPackRelease.sh` or `LinuxScripts/LinuxPackReleaseDevTools.sh` (creates the corresponding installer)

Windows installation and packaging support `Release` (without developer tools) and `ReleaseDevTools` (with developer tools). Build the selected configuration before installing or packaging. Both install helpers use the same destination; both packaging helpers write the configured installer filename to `runtime`, replacing the previous installer. Linux helpers are unchanged.

The Windows NSIS uninstaller automatically removes installation-local `logs` and `shader/cache` folders. It asks **Remove settings and user data?**, defaulting to **No**. Choosing **Yes** recursively deletes only the installation-local `config` folder; choosing **No** preserves the entire folder, including packaged settings files. Silent uninstallation preserves configs as well. If settings cannot be safely preserved, uninstallation stops instead of deleting them.

Cleanup does not follow custom paths from `path.ini` or recursively remove asset folders. Other package-owned files are removed normally; manually added assets and external folders are not additional cleanup targets. Linux packaging is unchanged.

## Assets
For the demo level, runtime assets should be downloaded separately and put under `$PROJECT_ROOT/runtime/assets`.
