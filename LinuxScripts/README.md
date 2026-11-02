# Linux scripts

Helpers for configuring, building, installing, and packaging the project on Linux. Platform-specific helpers use the `Linux…` naming convention, matching the `Windows…` helpers in `WinScripts`.

## Prerequisites and paths

Use a C++20-capable GCC or Clang toolchain, CMake 3.21 or newer for the presets, the build tool required by your generator, and the repository's initialized `ThirdParty/vcpkg` submodule. The dependency installer uses `apt`/`apt-get` and `sudo`, so it targets Debian/Ubuntu-based systems.

Helpers resolve the repository root from their own location and can be launched from any directory. The dependency installer only installs system packages and invokes the adjacent vcpkg bootstrap helper; it does not change the caller's directory. Use `bash` to invoke scripts that require Bash rather than running them with `sh`.

CMake configuration installs manifest dependencies into `ThirdParty/vcpkg_installed` using custom triplets from `ThirdParty/vcpkg_triplets`. There is no separate package-update step. Clean configuration preserves both directories.

The Linux presets are single-configuration, with separate build folders:

| Configuration | Build folder | Preset |
| --- | --- | --- |
| Release | `build` | `linux-release` |
| Debug | `build-debug` | `linux-debug` |
| ReleaseDevTools | `build-release-devtools` | `linux-release-devtools` |

## Script reference

| Script | Purpose |
| --- | --- |
| `LinuxInstallDependencies.sh` | Install OpenGL/X11 development packages and Mesa utilities, then bootstrap vcpkg. |
| `InstallVCPKG.sh` | Bootstrap the repository's vcpkg installation without installing system packages. |
| `LinuxConfig.sh` | Configure Release with `cmake --preset linux-release`; the preset enables tests and vcpkg manifest mode installs dependencies during configuration. |
| `LinuxConfigDebug.sh` | Configure with `cmake --preset linux-debug`. |
| `LinuxConfigReleaseDevTools.sh` | Configure with `cmake --preset linux-release-devtools`. |
| `LinuxCleanConfig.sh` | Delete `build`, `build-debug`, and `build-release-devtools`, then configure Release through `LinuxConfig.sh`. Installed dependencies and custom triplets are preserved. |
| `LinuxConfigCB.sh` | Configure Release with the `CodeBlocks - Unix Makefiles` generator. |
| `LinuxConfigCL.sh` | Configure Release with the `CodeLite - Ninja` generator. |
| `LinuxConfigCLDebug.sh` | Configure Debug with the `CodeLite - Ninja` generator. |
| `LinuxBuildRelease.sh` | Build with `cmake --build --preset linux-release`. |
| `LinuxBuildDebug.sh` | Build with `cmake --build --preset linux-debug`. |
| `LinuxBuildReleaseDevTools.sh` | Build with `cmake --build --preset linux-release-devtools`. |
| `LinuxInstallRelease.sh` | Install the built Release configuration to `runtime/install`. |
| `LinuxInstallReleaseDevTools.sh` | Install the built ReleaseDevTools configuration to `runtime/install`. |
| `LinuxPackRelease.sh` | Package Release using `cpack --config build/CPackConfig.cmake -C Release`. |
| `LinuxPackReleaseDevTools.sh` | Package ReleaseDevTools using `cpack --config build-release-devtools/CPackConfig.cmake -C ReleaseDevTools`. |
| `GetVersionNumber.sh` | Read the repository's `version.txt` and print the dotted version. |

## Usage

From the repository root, run `bash LinuxScripts/LinuxInstallDependencies.sh` once, then `bash LinuxScripts/LinuxConfig.sh` and `bash LinuxScripts/LinuxBuildRelease.sh`. If system dependencies are already installed, use `bash LinuxScripts/InstallVCPKG.sh` for bootstrap only.

For Debug or ReleaseDevTools, run the matching configure helper before its build helper. Run Release tests from the repository root with `ctest --preset linux-release`; for the other configurations use `ctest --preset linux-debug` or `ctest --preset linux-release-devtools`.

The IDE-specific configure helpers share the corresponding configuration's build folder. Clean or remove that folder before switching generators.

Build the selected configuration before installing or packaging. Both install helpers share `runtime/install` and replace its binaries with the selected configuration. CPack does not build the project or use `runtime/install`. Installers are written to `runtime` with the configured package filename, so packaging another configuration replaces the previous installer.
