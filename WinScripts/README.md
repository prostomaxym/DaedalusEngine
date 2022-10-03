## Windows specific scripts

Scripts (wrapers for regular terminal commands) for configuring, building, installing, packing project:
- CMakeConfig.bat - install dependencies and configure project files in ProjectRoot/Build folder
- CMakeCleanConfig.bat - delete Build folder and make clean project configuration
- CMakeBuildRelease.bat - build source files using Release configuration
- CMakeInstall.bat - install project files to ProjectRoot/Install folder
- CPackInstaller.bat - pack installer into 
- GetVersionNumber.ps1 - get version number from version.txt in project root
- InstallVCPKG.ps1 - run vcpkg bootstrap script
