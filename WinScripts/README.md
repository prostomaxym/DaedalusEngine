## Windows specific scripts

Scripts (wrappers for regular terminal commands) for configuring, building, installing and packing the project. Each one changes to the project root first.

- InstallVCPKG.ps1 - bootstrap vcpkg (first time only)
- UpdatePackages.bat - install vcpkg packages (`x64-windows` triplet, overlay triplets from `ThirdParty/vcpkg_triplets`)
- CMakeConfig.bat - configure project files in `ProjectRoot/build` (`cmake --preset windows`)
- CMakeConfigAndPackages.bat - same as CMakeConfig.bat, for the first configuration
- CMakeCleanConfig.bat - delete `build` folder and make a clean project configuration
- CMakeBuildRelease.bat - build using the Release configuration (`cmake --build --preset windows-release`)
- CMakeInstallRelease.bat - install the built Release configuration to `ProjectRoot/runtime/install`
- CMakeInstallReleaseDevTools.bat - install the built ReleaseDevTools configuration to `ProjectRoot/runtime/install`
- CPackInstallerRelease.bat - package the built Release configuration without developer tools (`cpack --config build/CPackConfig.cmake -C Release`)
- CPackInstallerReleaseDevTools.bat - package the built ReleaseDevTools configuration with developer tools (`cpack --config build/CPackConfig.cmake -C ReleaseDevTools`)
- GetVersionNumber.ps1 - get version number from `version.txt` in the project root

Debug and ReleaseDevTools builds: use the generated `.sln` or `cmake --build --preset windows-debug` / `windows-release-devtools`.

The install helpers can be launched from any directory. Build the selected configuration first; both helpers share the same installation destination and replace its binaries with the selected configuration.

The packaging helpers can also be launched from any directory. Build the matching configuration before packaging; CPack does not build it or use `runtime/install`. Installers are written to `runtime` with the configured package filename, so packaging another configuration replaces the previous installer.
