## Linux specific scripts

Scripts (wrappers for regular terminal commands) mirroring `WinScripts`. Each one changes to the project root first, so they can be run from any directory.

Linux generators are single-configuration, so each configuration uses its own build folder: `build` (Release), `build-debug` (Debug), `build-release-devtools` (ReleaseDevTools).

- InstallDependencies.sh - install system packages (OpenGL, X11 dev libraries) and bootstrap vcpkg
- InstallVCPKG.sh - bootstrap vcpkg (first time only)
- UpdatePackages.sh - install vcpkg packages (`x64-linux` triplet)
- CMakeConfig.sh / CMakeConfigAndPackages.sh - configure Release (`cmake --preset linux-release`)
- CMakeConfigDebug.sh - configure Debug (`cmake --preset linux-debug`)
- CMakeConfigReleaseDevTools.sh - configure ReleaseDevTools (`cmake --preset linux-release-devtools`)
- CMakeCleanConfig.sh - delete all build folders and make a clean Release configuration
- CMakeConfigCB.sh / CMakeConfigCL.sh / CMakeConfigCLDebug.sh - configure for Code::Blocks / CodeLite
- CMakeBuildRelease.sh / CMakeBuildDebug.sh / CMakeBuildReleaseDevTools.sh - build the matching configuration
- CMakeInstall.sh / CMakeInstallReleaseDevTools.sh - install Release / ReleaseDevTools to `runtime/install`
- CPackInstaller.sh / CPackInstallerReleaseDevTools.sh - pack Release / ReleaseDevTools package
- GetVersionNumber.sh - print version number from `version.txt`
