# Windows scripts

Helpers for Windows desktop builds and for building and deploying the Android sandbox from Windows.

## Windows desktop

Prerequisites: Visual Studio with C++ build tools, CMake 3.21 or newer for the presets, and the repository's initialized `ThirdParty/vcpkg` submodule.

Bootstrap, configure, build, clean, install, and packaging helpers resolve paths from their script locations and can be launched from any directory. The version helper is the exception noted below.

CMake configuration installs manifest dependencies into `ThirdParty/vcpkg_installed` using custom triplets from `ThirdParty/vcpkg_triplets`. There is no separate package-update step. Clean configuration preserves both directories.

Exit Visual Studio before clean configuration if it holds IDE caches open inside a build folder. Closing only the solution may not release those files; the clean helper reports failure instead of configuring over a partially deleted build.

| Script | Purpose |
| --- | --- |
| `InstallVCPKG.ps1` | Bootstrap the repository's vcpkg installation (first time only). |
| `WindowsConfig.bat` | Configure `build` with `cmake --preset windows`; the preset enables tests and vcpkg manifest mode installs dependencies during configuration. |
| `WindowsBuildRelease.bat` | Build with `cmake --build --preset windows-release`. |
| `WindowsBuildReleaseDevTools.bat` | Build with `cmake --build --preset windows-release-devtools`. |
| `WindowsInstallRelease.bat` | Install the built Release configuration to `runtime/install`. |
| `WindowsInstallReleaseDevTools.bat` | Install the built ReleaseDevTools configuration to `runtime/install`. |
| `WindowsPackRelease.bat` | Package Release without developer tools using `cpack --config build/CPackConfig.cmake -C Release`. |
| `WindowsPackReleaseDevTools.bat` | Package ReleaseDevTools with developer tools using `cpack --config build/CPackConfig.cmake -C ReleaseDevTools`. |
| `WindowsCleanConfig.bat` | Delete `build`, `build-debug`, and `build-release-devtools`, then run `WindowsConfig.bat`. Installed dependencies and custom triplets are preserved. |
| `GetVersionNumber.ps1` | Read `version.txt` relative to the current directory and return the dotted version. Run this helper from the repository root. |

Typical sequence from `WinScripts`: run `./InstallVCPKG.ps1`, `./WindowsConfig.bat`, then `./WindowsBuildRelease.bat` (or `./WindowsBuildReleaseDevTools.bat`).

For Debug, use the generated solution or run `cmake --build --preset windows-debug` from the repository root. Run Release tests from the root with `ctest --preset windows-release`.

Build the selected configuration before installing or packaging. Both install helpers share `runtime/install` and replace its binaries with the selected configuration. CPack does not build the project or use `runtime/install`. Installers are written to `runtime` with the configured package filename, so packaging another configuration replaces the previous installer.

## Android from Windows

The Android batch wrappers call the PowerShell helpers in `Android/`, resolve paths relative to their scripts, and forward additional arguments. They can be launched from any directory.

Prerequisites:

- The repository's initialized and bootstrapped vcpkg submodule.
- Android SDK platform 33, Build Tools `33.0.2`, platform-tools (`adb`), NDK `27.2.12479018`, and SDK CMake `3.22.1` (including Ninja).
- JDK 11, selected through `JAVA_HOME` or `-JavaHome`, and Gradle `7.3.3`. The build helper looks for `gradle.bat` on `PATH`, then in the Gradle wrapper cache for `7.3.3`; otherwise pass `-GradlePath`.
- For deployment or probing, a connected and authorized Android device. The sandbox targets `arm64-v8a` and requires Android 9 / API 28 or newer.

SDK lookup defaults to `ANDROID_HOME`, otherwise `%LOCALAPPDATA%/Android/Sdk`. Pass `-SdkRoot` to override it.

| Script | Purpose |
| --- | --- |
| `AndroidDependencies.bat` | Install `arm64-android` dependencies through `Android/Dependencies.ps1`. Supports `-SdkRoot`, `-NdkVersion`, `-InstalledDirectory`, `-WorkDirectory`, and `-DryRun`. |
| `AndroidBuildRelease.bat` | Build Release native libraries and assemble the sandbox APK through `Android/Build.ps1`. |
| `AndroidBuildReleaseDevTools.bat` | Build ReleaseDevTools native libraries and assemble the sandbox APK. |
| `AndroidBuildAndDeploy.bat` | Build ReleaseDevTools, then install and launch it on a device. |
| `AndroidDeploy.bat` | Install an existing APK through `Android/Deploy.ps1` with `adb install -r`, force-stop and launch `org.daedalus.sandbox`, then check that its process is running. Existing app data is preserved. Supports `-SdkRoot`, `-ApkPath`, and `-Serial`. |
| `Android/Probe.ps1` | Build and run an isolated GLES/shader feasibility probe on a device, then remove its temporary device files. Does not install an application or validate NativeActivity lifecycle or rendering performance. Supports `-SdkRoot`, `-NdkVersion`, `-CMakeVersion`, `-Serial`, and `-BuildDirectory`. |

Typical sequence: run `./AndroidDependencies.bat`, then `./AndroidBuildRelease.bat` and `./AndroidDeploy.bat`, or use `./AndroidBuildAndDeploy.bat` for a ReleaseDevTools build and deployment. Invoke these paths from `WinScripts`; use full paths from elsewhere. Run the probe directly with `powershell.exe -NoProfile -ExecutionPolicy Bypass -File ./Android/Probe.ps1`.

Build wrappers accept `-SdkRoot`, `-NdkVersion`, `-CMakeVersion`, `-InstalledDirectory`, `-WorkDirectory`, `-GradlePath`, `-JavaHome`, `-AssetDirectory`, `-ConfigDirectory`, `-PathConfigFile`, `-Deploy`, and `-Serial`. When customizing dependency storage, pass the same `-InstalledDirectory` to the dependency and build helpers, and use the same NDK version for both.

Runtime bundling is opt-in: from the repository root, run `./WinScripts/AndroidBuildRelease.bat -AssetDirectory ./runtime/asset`. Supply the asset root with the scene, all referenced resources and UUID `.meta` sidecars. Optional `-PathConfigFile` and `-ConfigDirectory` add Android-compatible path configuration and settings. Startup copies missing files only: existing device assets/configuration are preserved even when the APK changes. Shaders still refresh normally. Default commands do not bundle runtime assets. See [Android runtime bundling](../.github/docs/Android.md#bundle-runtime-assets-in-the-apk) for input layout and update behavior.

**Dependency isolation:** Android defaults to `ThirdParty/vcpkg_installed_android`; desktop retains `ThirdParty/vcpkg_installed`. Each root has its own vcpkg package database, including any host-tool packages. Separate triplet folders within one root do not isolate manifest reconciliation: an existing Windows manifest-install log confirms removal of Android packages from the former shared root. Explicit `-InstalledDirectory` overrides remain supported; select a root separate from desktop and pass it to both Android helpers. Do not delete or move the existing installation. Android APK builds use `VCPKG_MANIFEST_MODE=OFF` and do not install dependencies. `WindowsCleanConfig.bat` deletes only build folders, but its subsequent Windows configuration can reconcile packages in the desktop root. Both installed-package roots and custom triplets must be preserved during cleanup.

The new Android default root is not populated automatically by this change; install its dependencies before building, or select an existing isolated Android installation with `-InstalledDirectory`. Dependency buildtrees/packages remain under `%TEMP%/DaedalusAndroidDependencies`; native builds and Gradle output default to the repository's `runtime/DaedalusAndroidApk`. Both native configurations are packaged with Gradle's `assembleDebug`, producing a debug APK at `runtime/DaedalusAndroidApk/gradle/app/outputs/apk/debug/app-debug.apk`, not a signed release APK. Standalone deployment defaults to that same repository-relative APK. If you change the build work directory, pass the resulting APK path to `AndroidDeploy.bat -ApkPath` (build-and-deploy forwards it automatically).

Deployment selects the only authorized device unless `-Serial` is provided; specify a serial when multiple devices are connected.
