## Windows specific scripts

Scripts (wrappers for regular terminal commands) for configuring, building, installing and packing the project. Each one changes to the project root first.

- InstallVCPKG.ps1 - bootstrap vcpkg (first time only)
- UpdatePackages.bat - install vcpkg packages (`x64-windows` triplet, overlay triplets from `ThirdParty/vcpkg_triplets`)
- CMakeConfig.bat - configure project files in `ProjectRoot/build` (`cmake --preset windows`)
- CMakeConfigAndPackages.bat - same as CMakeConfig.bat, for the first configuration
- CMakeCleanConfig.bat - delete `build` folder and make a clean project configuration
- CMakeBuildRelease.bat - build using the Release configuration (`cmake --build --preset windows-release`)
- CMakeInstall.bat - install project files to `ProjectRoot/runtime/install` (Release)
- CPackInstaller.bat - pack the installer with `cpack` from the `build` folder
- GetVersionNumber.ps1 - get version number from `version.txt` in the project root

Debug and ReleaseDevTools builds: use the generated `.sln` or `cmake --build --preset windows-debug` / `windows-release-devtools`.
