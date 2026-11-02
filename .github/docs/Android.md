# Android sandbox: build and validation

## Current status

**Initial Android port complete and user-verified**, with skybox rendering as the sole known initial-port exception. The user accepted the deployed Samsung SM-S931B build after scene rendering, lifecycle, touch controls and the Android debug overlay checks. This acceptance supersedes the earlier pending manual checks recorded in the historical investigation below. It does not claim exhaustive forced EGL context-loss testing, additional device coverage or Play Store readiness. The skybox issue remains paused; no depth workaround is applied.

## Quick Windows commands

Run from the repository root after installing the prerequisites below and setting `ANDROID_HOME` and `JAVA_HOME` (JDK 11). Bootstrap the pinned vcpkg submodule once with `ThirdParty\vcpkg\bootstrap-vcpkg.bat`. Install Gradle 7.3.3 on PATH, use its existing wrapper cache, or pass `-GradlePath`.

| Entry point | Purpose |
|---|---|
| `WinScripts\AndroidDependencies.bat` | Build Android dependencies once (repeat after manifest/toolchain changes) |
| `WinScripts\AndroidBuildRelease.bat` | Configure/build/package production engine configuration in a debug-signed APK |
| `WinScripts\AndroidBuildReleaseDevTools.bat` | Build APK with console/debug tools |
| `WinScripts\AndroidDeploy.bat` | Install the latest APK and relaunch, preserving private data |
| `WinScripts\AndroidBuildAndDeploy.bat` | Build developer APK, install and launch in one command |

All entry points forward optional parameters and propagate failures. To reuse an existing isolated Android installation, pass `-InstalledDirectory "C:\path\to\android-installed"` to both dependency and build entry points. A fresh default setup uses `ThirdParty/vcpkg_installed_android` for installed packages, separate from desktop. Dependency buildtrees/packages remain under `%TEMP%/DaedalusAndroidDependencies`; APK build output defaults to `runtime/DaedalusAndroidApk`. Set `JAVA_HOME` to the installed JDK 11 before invoking the batch shortcuts, or pass `-JavaHome` to build commands.

PowerShell equivalents are `./WinScripts/Android/Build.ps1 -Configuration ReleaseDevTools -Deploy` and `./WinScripts/Android/Deploy.ps1`. Deployment requires USB debugging and an authorized device; multiple devices require `-Serial`. It validates install, activity launch and process presence, not visible rendering. It never uninstalls, clears data or provisions assets. A custom build `-WorkDirectory` requires passing the resulting `-ApkPath` to standalone deployment; integrated `-Deploy` selects it automatically. Deployment installs whichever configuration last wrote the APK at that path.

## GitHub Actions

`.github/workflows/Android-CI.yml` runs for pushes and pull requests targeting `development` or `master`, matching existing desktop CI branches. On a Windows runner it installs the pinned SDK/NDK/CMake/JDK/Gradle versions, caches vcpkg binaries, builds Android dependencies and both Release/ReleaseDevTools APKs, then builds/runs the Windows host gtest suite through CTest. Android root CMake still disables the full engine test target; CI does **not** execute ARM binaries, run an emulator, supply private assets or assert device rendering/lifecycle correctness. No APK publishing/signing secrets or artifact-release pipeline is configured. The workflow's first hosted run remains to be verified after pushing.

### Skybox investigation paused

Paused at the user's request with the skybox still not visible. Temporary skybox diagnostics have been removed: extra query draws, cubemap readbacks, depth-probe shaders, clear/copy instrumentation and scene/path logs. Normal renderer operations, the 36-vertex correction and GLES compatibility fixes remain; no additional skybox fix or depth workaround is applied. The evidence below is historical and records an unresolved issue, not a verified rendering fix. Probe helpers named below are no longer in the current source. The cleaned-up developer APK was subsequently deployed and the user accepted the initial port except for skybox.

- Confirmed limitation: the latest detailed probes inspect the initial 1080x2237 portrait frame, not the recreated 2237x1080 landscape buffers the user subsequently observes.
- Unverified hypothesis: lighting writes depth 0.5 before the G-buffer depth copy; an incorrect or failed copy after resize could leave that depth and reject the far-depth skybox. No settled-landscape evidence establishes this, so it must not be treated as the root cause or justification for a depth-state change.
- If resumed: correlate actual skybox color, background depth/coverage and pre-swap framebuffer contents on the same settled landscape frame. Check surface/G-buffer dimensions and copy results after recreation, and compare equivalent frames with and without probes. A passing boolean query alone is insufficient.
- This pause is documentation-only; no new build, tests, APK deployment or device validation was performed.
- Subsequent diagnostic cleanup validation: Windows build and Android ReleaseDevTools native build passed; Windows unit tests passed 464 of 467 with 3 skipped. Android compilation retains existing warnings. No APK deployment, post-cleanup device run or Linux/WSL validation was performed.

### Recorded implementation and device evidence

The target is an arm64-v8a sandbox using NativeActivity, EGL and OpenGL ES 3.2, without a new platform library. Android engine/client libraries and a signed debug APK build successfully with NDK 27.2. On Samsung SM-S931B / Adreno 830 / API 36, installation/startup succeeded and the user confirmed the manually provisioned Nuke scene, lifecycle checks and touch controls work. The initial port is accepted except for skybox; exhaustive forced EGL context-loss recovery is not separately established. Windows/Linux retain the desktop backend.

Skybox follow-up: the out-of-bounds draw (108 float components submitted as 108 vertices instead of 36) is fixed, with immutable geometry and a compile-time size guard. The parent execution session reports a successful Windows build, 467 ReleaseDevTools tests (464 passed, 3 skipped), and successful Android ReleaseDevTools diagnostic APK rebuild/install/start with private assets preserved. The user explicitly confirms the skybox is **still not visible**; the count fix is not a verified root cause. Scene geometry, movement and touch controls are visible. The actual installed private `path.ini` selects `scenes/nuke.scene.json`; that JSON contains entity 9, Cubemap, six Night face paths and rotation 0. Runtime diagnostics independently confirm one component and those six source paths. No runtime assets were changed.

Source tracing confirms `RenderSystem` draws skybox components after deferred lighting and the G-buffer depth copy. PID 12694's complete host-captured log verifies default depth24/stencil8, a successful complete-FBO depth blit (10 to 0, 2237x1080), six 512x512 RGB8 allocations on texture 165, validated program 6, 36 vertices, unit 0 with no sampler object, LEQUAL depth testing, back-face culling and enabled color writes. The inspected blit and draw return no GL error. This does not establish uploaded texel contents, fragment survival or visible output.

The full log also identifies pre-existing invalid operations: startup mipmap generation, shadow-array texture 196 bound to GL_TEXTURE_2D during debug computation, and four image-uniform assignments per frame. Follow-up source changes remove unnecessary single-level float-texture mipmap generation, use the array binding API for the debug shadow conversion, and retain GLES image bindings from shader layout qualifiers rather than glUniform. These are GL-error fixes, not a proven skybox visibility fix. The parent reports those changes compiled successfully on Windows and Android and were installed without clearing assets.

Earlier evidence: the saved complete all-buffer log `C:/Users/ershi/AppData/Local/Temp/Daedalus-skybox-13709.log` (lines 1514–1552) confirms depth24/stencil8, a complete/error-free 4→0 depth blit at initial portrait resolution 1080x2237, and nonblack GPU readbacks at two positions on every cubemap face with no read errors. Program 6 validates; the draw uses 36 vertices, LEQUAL, depth writes enabled, expected masks/range and no GL error. Non-writing `GL_ANY_SAMPLES_PASSED` queries return baseline=0, no-cull=0, no-depth=1, neither=1 (boolean results, not fragment counts). This establishes depth rejection for that instrumented draw only. It does **not** distinguish bad source/copied depth from far-plane rasterization precision or establish the behavior of another build/frame. The earlier landscape PID 12694 capture and this initial portrait capture must not be conflated.

Source tracing: `DeferredLightPass` clears the default color/depth buffers and draws a z=0 fullscreen quad, normally writing window depth 0.5. `Renderer::FlushPipeline` subsequently copies G-buffer depth to the default framebuffer, then runs debug compute. `RenderSystem` then draws the skybox with `gl_Position = pos.xyww`, intended window depth 1. Thus the lighting quad is not sufficient to explain rejection unless its depth survives the later copy; framebuffer completeness and no GL error do not prove depth contents.

Latest executed diagnostics: the parent reports successful Windows and Android ReleaseDevTools builds, 467 tests (464 passed / 3 skipped), replacement installation preserving private files, and launch as PID 15110. The user explicitly confirms this deployed depth-probe build **still has no visible skybox**. Direct inspection of `C:/Users/ershi/AppData/Local/Temp/Daedalus-skybox-15110.log` gives different results from PID 13709:
- Lines 1482–1487: the first three clears (FBOs 7, 4 and 0) have depth writes enabled, clear depth 1, scissor disabled and no GL error. Explicit depth-1 equality/LEQUAL queries pass, depth-0.5/0 equality queries fail, and the nine-point far grid is `0x1ff`.
- Lines 1488–1491: geometry FBO 4 has some depth-1 survivors but far grid `0x0`. Lighting FBO 0 instead has depth-0.5 survivors and no depth-1 survivors. After the error-free complete-FBO 4→0 depth copy, destination queries match the geometry source: depth-1 equality/LEQUAL pass, depth-0.5/0 equality fail, and far grid remains `0x0`.
- Lines 1492–1520: all six cubemap faces again have valid, nonblack RGB8 readbacks; program 6 validates and the actual draw has no GL error. Immediately before skybox, after debug compute, the destination depth results remain unchanged. Actual-skybox queries now return baseline=1, no-cull=1, no-depth=1, neither=1 and exact-far-range=1. These are boolean survival results with color/depth writes suppressed, not coverage counts or proof of sampled/displayed color.
- A zero nine-point grid is consistent with a passing whole-viewport query: the grid tests only nine one-pixel positions, all of which may contain geometry. The matching results do not prove an identical full depth image or substantial visible background coverage.
- These detailed results precede the first presented frame (line 1521). Lines 1534–1545 then show a surface resize to **2237x1080 landscape** and renewed default-depth format queries during framebuffer recreation. The clear/copy/skybox diagnostics use static one-shot counters/flags and do not repeat for those recreated buffers. The passing portrait query must not be treated as evidence about the later landscape frame the user observes.

Probe state-restoration source audit (not runtime validation):
- `OpenGLFramebuffer::DiagnoseDepthImpl` restores the current program, VAO, draw framebuffer, viewport, scissor box, depth function/write mask, every indexed color mask, and all seven capability enables it changes (depth, cull, scissor, stencil, rasterizer discard, alpha-to-coverage and sample coverage). It uses a temporary VAO/program/query, disables depth/color writes and stencil testing, and does not change read-framebuffer binding, depth range, texture bindings or blend state. Shader compilation/linking does not bind the probe program. No missing restoration of explicitly mutated renderer state was identified.
- `DiagnoseSkyboxFragments` restores depth/cull enables, depth range, color mask 0, depth write mask and separate front/back stencil write masks. Its query draws do not write color/depth/stencil. Other indexed color masks and the depth function are not changed.
- `DiagnoseCubemapTexels` restores the read framebuffer, pixel-pack buffer and all four pixel-pack parameters it changes. `glReadBuffer` applies to its temporary read FBO; the previous FBO's read-buffer selection is not modified. The caller restores the active texture unit and never changes the cubemap binding during inspection.
- The helpers still insert extra draws, shader compilation, readbacks and synchronous query-result waits. Restoring API state does not establish that these operations cannot perturb driver execution or early/late depth behavior. The change between PIDs is not proof of either a state leak or a fixed depth path. No before/after runtime state snapshot or probe-free equivalent-frame comparison has been obtained.

Source review also confirms a direct cubemap texture fragment output, unit-0 cube binding with no sampler object in the logged draw, and the order skybox → optional debug lines → ImGui → EGL swap. No unconditional post-skybox clear was found in that application path; touch controls draw localized foreground primitives. This does not establish the final framebuffer contents, coverage, or presentation of the failing frame. The root cause remains unresolved. Do not disable skybox depth testing or call this build visually fixed. The next decisive evidence must correlate actual color output and background depth/coverage on the same settled landscape frame, through pre-swap presentation, with probe restoration/nonperturbation checked; another successful boolean query alone is insufficient. This follow-up session has no terminal/ADB/build tool, made no renderer changes, and performed no new build, test run or device validation.

Inspect complete new-process logs, not just error-keyword matches. No runtime assets, shader caches or old logs should be cleared for this investigation.

The standalone `source/dlAndroidProbe` project is deliberately not added to the root build or the desktop unit tests. It uses only NDK libraries and exercises the device GPU without loading the engine, runtime scenes or binary assets. Native diagnostics use Android logcat rather than the engine logger, which is not linked into this probe.

## Reproduce the device probe on Windows

Install Android SDK platform-tools, NDK `27.2.12479018` (NDK 25.1 libc++ lacks C++20 concepts required by EnTT), and SDK CMake `3.22.1`. Enable USB debugging, authorize the development computer, and connect an arm64 device with GLES 3.2 support.

From the repository root, run `./WinScripts/Android/Probe.ps1`. With multiple devices, pass `-Serial <adb-device-serial>`. SDK and tool versions can be overridden with `-SdkRoot`, `-NdkVersion`, and `-CMakeVersion`.

The helper builds into a version-specific directory under the host temporary directory by default. It pushes an executable and shader sources into a unique `/data/local/tmp/daedalus-probe-*` directory, prints that process's logcat diagnostics, propagates failure, and removes its device directory. It does not install an application or change device settings. No generated output belongs in source control.

The probe uses a GLES 3.2 EGL pbuffer context and checks:
- Required limits: 32 geometry invocations, two fragment and two geometry SSBO blocks, six draw buffers/color attachments, 32 array layers and a 2048-pixel texture dimension.
- Representative geometry/SSBO and compute program compilation/linking.
- RGBA8, RGBA16F, R16F and R32I color framebuffer completeness.
- Combined G-buffer completeness: two RGBA16F, three RGBA8, one R16F and a depth32F attachment.
- A 32-layer depth32F framebuffer with border clamp.
- Compilation/linking of all 11 engine shader programs, including includes, using a probe-only GLSL ES 320 preamble with float, integer, sampler and image precision declarations.

Production shader files retain desktop GLSL 450 declarations. Numeric conversions and runtime-initialized local variables satisfy both desktop and ES language rules. On Android, both initial compilation and hot reload replace the version with a GLSL ES 320 precision preamble. Binary-cache fingerprints include that dialect and GL vendor/renderer/version identity. Five graphics-independent tests cover dialect conversion; the device probe does not establish engine rendering correctness.

## Android dependencies

Run `./WinScripts/Android/Dependencies.ps1` to cross-build the root manifest for `arm64-android`. `-DryRun` checks dependency selection without installing packages. SDK/NDK location, installed packages and the temporary working directory are configurable with `-SdkRoot`, `-NdkVersion`, `-InstalledDirectory`, and `-WorkDirectory`.

**Install-root isolation:** Android now defaults to `ThirdParty/vcpkg_installed_android`, separate from desktop's `ThirdParty/vcpkg_installed`. Separate triplet folders alone do not isolate the package database: manifest installation reconciles the entire root to the selected target/host graph, and existing Windows configuration logs confirm removal of Android packages from the former shared root. Explicit Android `-InstalledDirectory` overrides must also remain separate from desktop. Existing installed packages are not deleted or migrated by this change; populate the new default root before building, or reuse an existing isolated Android installation. Preserve both roots during cleanup. Android `Build.ps1` disables manifest mode; the dependency installer and Windows configuration are the reconciliation entry points.

The helper uses the repository's pinned vcpkg executable/manifest and overlay triplets from `ThirdParty/vcpkg_triplets`, targeting `arm64-android` with `x64-windows` host tools in the Android install root. Temporary buildtrees and packages default to `%TEMP%/DaedalusAndroidDependencies`. The NDK environment override is restored on completion.

On Android, the manifest excludes GLFW and desktop GLAD and selects ImGui's Android/OpenGL3 backends with docking. Desktop dependency features and the manifest baseline are unchanged. The Android overlay inherits the pinned vcpkg triplet (API 28, static dependencies, shared libc++) and disables Assimp's unused 3MF exporter to avoid its kubazip/libzip symbol collision, matching the existing Linux integration rationale. The APK packages one matching `libc++_shared.so`; the isolated single-executable GPU probe instead uses static libc++.

## Build the debug APK on Windows

Install JDK 11, Gradle 7.3.3, SDK platform 33 and build-tools 33.0.2, in addition to the native prerequisites above. `Android/` pins Android Gradle Plugin 7.2.0. This local development baseline emits compatibility warnings for compileSdk 33 and newer SDK metadata; it is not a Play Store publishing configuration.

The helper discovers Gradle on PATH or an existing Gradle 7.3.3 `-all` distribution under the user's Gradle wrapper cache. Use Gradle 7.3.3 with this pinned AGP baseline. It uses `JAVA_HOME` when `-JavaHome` is omitted. No wrapper binary or downloaded tools are committed.

Dependencies must have been built using the same NDK. Both Android helpers default to `ThirdParty/vcpkg_installed_android`; to use another isolated installation, pass the same `-InstalledDirectory` to `Dependencies.ps1` and `Build.ps1`. Override SDK/NDK/CMake paths with `-SdkRoot`, `-NdkVersion` and `-CMakeVersion` as needed.

The helper configures a separate Release native build, stages `libdlEngine.so`, `libdlSandbox.so`, the selected NDK's `libc++_shared.so`, and root-level `.glsl`/`.hglsl` files, then runs `assembleDebug`. Native libraries link with `ANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON` (16-KB LOAD alignment, verified with `llvm-readelf`) to avoid the Android 16-KB page compatibility warning. Runtime asset/config bundling is opt-in as described below. Native debug symbols are retained for local diagnostics. Unique staging folders are removed on completion; build/cache/output files live under the repository's `runtime/DaedalusAndroidApk` (override `-WorkDirectory`). The APK is `gradle/app/outputs/apk/debug/app-debug.apk` below that directory; standalone deployment defaults to the same APK (override `-ApkPath`). It is debug-signed, debuggable, arm64-only, min API 28 and target API 33, and declares GLES 3.2 as required. No runtime permission is requested.

When a device is connected, install with `adb install -r <apk>` and launch with `adb shell am start -n org.daedalus.sandbox/android.app.NativeActivity`. Installation and launch have been device-validated with existing private assets/configuration preserved. The native entry extracts bundled shaders to the application's private `shader/` directory, creates `asset/`, `config/` and `logs/`, and uses the private data directory as the working directory. Engine logs go to logcat with tag `Daedalus`. Models, textures, fonts and the configured scene must be supplied manually or explicitly bundled during the build; the default APK still does not provide a playable scene.

### Bundle runtime assets in the APK

From the repository root, build an APK containing your asset tree:

```powershell
.\WinScripts\AndroidBuildRelease.bat -AssetDirectory .\runtime\asset
```

`-AssetDirectory` identifies the asset root itself, not its parent. All nested files, including UUID `.meta` sidecars and dotfiles, are included under the private `asset/` directory. Links/junctions and filenames containing newlines are rejected. No reference scene or external resource is added automatically: supply the scene and every model, texture, skybox and font it references. Asset paths and filename case must match on Android. Without a custom path config the startup scene is `asset/scenes/startup.scene.json`.

Optional `-PathConfigFile` and `-ConfigDirectory` bundle an Android-compatible `path.ini` and the contents of `config/` respectively:

```powershell
.\WinScripts\AndroidBuildReleaseDevTools.bat -AssetDirectory .\runtime\asset -PathConfigFile .\android-runtime\path.ini -ConfigDirectory .\android-runtime\config -Deploy
```

These are manually supplied input paths; create them before invoking the command. Use relative runtime paths in the INI, such as `[Path]` keys `AssetsFolder=asset`, `ShadersFolder=shader`, `LogsFolder=logs`, `SceneFile=scenes/startup.scene.json`, and `[Config]` key `ConfigsFolder=config`. Do not reuse Windows absolute paths. All three options also work with `WinScripts/Android/Build.ps1` and the other APK build wrappers. The input asset/config directories must not contain the APK work/staging directory.

The build stages a UTF-8 `runtime-files.txt` manifest and a separate `runtime/` asset tree. Before constructing the engine, Android startup streams **only missing files** into its private runtime directory. Existing files are never replaced, including assets, sidecars and configuration; files removed from later APKs are not deleted from the device. Extraction uses temporary files, re-checks the destination and publishes by atomic rename (app storage denies hard links), so failed copies can be retried on the next launch without accepting partial destinations. Unsafe paths and symlink destinations fail startup with a logcat error. Shaders retain their existing refresh-on-launch behavior.

Installing a newer APK with `adb install -r` preserves existing runtime files. To use a changed bundled file, remove that specific private file via ADB before relaunching, or clear application storage for a completely fresh installation (**this deletes all user assets/configuration**). Bundled content occupies APK storage as well as extracted storage. This feature does not add a file picker, USB/MTP access to private storage, or asset cooking.

Bundling validation: Windows ReleaseDevTools build and full CTest passed (491 passed, 4 skipped out of 495; symlink tests require Windows privileges). The 14 installer cases include preservation, failed-copy retry, unsafe paths and existing-file preservation; 13 ran and the symlink case skipped. PowerShell fixture checks passed for staging, hidden files, sidecars, configs, opt-in defaults, invalid input and staging-recursion rejection. An isolated ReleaseDevTools Android native build and Gradle debug APK assembly passed; archive inspection verified all six fixture manifest entries (nested asset, sidecar, dotfile, scene and two configs), plus the original 13 shaders and three native libraries. The validation APK contains fixtures, not a playable scene. The first device launch failed with `Permission denied` because Android forbids hard links in app storage; publication now uses rename. Linux/WSL validation was deferred.

Touch controls use a hard-coded sandbox layout: lower-left movement stick, right-side look drag, lower-right **Up/Down**, and top-row **Console / Debug / POV / Light**. Movement synthesizes configured MoveForward/Backward/Left/Right keys with a 0.25 threshold. Up/Down hold the configured MoveUp/MoveDown keys; this is camera movement, not a grounded physics jump. POV/Light latch release taps into ChangePOV/Flashlight key pulses. Console/Debug queue the existing overlay key-release events and are shown only in developer builds. Missing overlay bindings receive grave-accent/F3 defaults; existing bindings are preserved.

The deployed expanded-control developer APK is covered by the user's initial-port acceptance.

Gamepad, soft-keyboard text entry, runtime capability diagnostics and detailed surface/context-error recovery testing remain open. GPU timestamps are explicitly unavailable on Android rather than reported as measurements.

## Android debug-overlay statistics

The General tab identifies the device manufacturer/model, Android version/API, ABI, SoC (hardware-property fallback), online logical CPU count, GL renderer and GLES version/driver string. Missing properties use descriptive fallbacks where available; GPU strings are queried on the graphics thread.

Application CPU load is process CPU-time growth divided by monotonic elapsed time and online logical CPU count (0–100% of that capacity). The initial sample is unavailable. System-wide CPU usage is unavailable to this implementation; GPU utilization, clocks, temperature, power and shared-memory usage/budget are also explicitly unavailable, not zero measurements or dedicated VRAM. No privileged/vendor-specific counters are used.

The updated APK was subsequently deployed; the user supplied an overlay screenshot and accepted the initial port with unavailable counters documented as expected. These changes do not resolve or resume the paused skybox investigation.

## Verified baseline

- Device: Samsung SM-S931B, arm64-v8a, Android API 36, 4096-byte pages.
- GPU: Adreno 830, GLES 3.2, Qualcomm driver `V@0800.64.7`.
- Actual limits: 32 geometry invocations, 1024 geometry output vertices, 12 fragment SSBOs, 4 geometry SSBOs, 8 draw buffers/color attachments, 2048 array layers and 16384 maximum texture dimension.
- Device probe: all listed shader and framebuffer checks passed.
- Dependencies: all 26 packages in the isolated Android dependency graph cross-built successfully in Debug/Release where supplied, using NDK 27.2 and API 28.
- Engine: `libdlEngine.so` and `libdlSandbox.so` (arm64-v8a, Release, `c++_shared`) compile and link against those dependencies.
- Packaging: Release and ReleaseDevTools native builds and Gradle debug APK assembly passed. APK signature verification passed for the initial package; the manifest/NativeActivity export and exactly three native libraries plus 13 shader/include assets were checked. No scene/config/font assets were bundled.
The user subsequently declared the initial port working and verified except skybox. Exhaustive forced EGL context loss remains separate future hardening.
- Windows: the expanded touch integration's ReleaseDevTools build passed with 464 tests passed and three skipped out of 467. Earlier Release/ReleaseDevTools CTest passes predate the final integration edits; a new explicit Release regression remains pending. Linux/WSL validation was deferred.

The initial-port device behavior is user-accepted except skybox; this is not exhaustive GPU correctness/performance testing, forced context-loss recovery, emulator coverage or 16-KB-page compatibility certification.

## Remaining implementation

The initial port is complete with user device signoff except the paused skybox defect. Future work includes manual asset provisioning tooling, gamepad/soft-keyboard/customizable UI, capability/error handling and forced-context-loss hardening, dedicated Android-executed tests, broader device coverage and release/distribution readiness. These are follow-up enhancements, not blockers to the accepted initial scope.

Finalization validation: all Android PowerShell scripts parsed; dependency dry-run passed; both new Release and ReleaseDevTools batch build entry points successfully packaged APKs. The deployment shortcut installed the developer build with data preserved and verified activity/process startup on Samsung SM-S931B; an invalid serial was rejected before installation. Windows build passed and the full host suite passed 469 tests with 3 skipped (472 total). The GitHub workflow is provided but has not yet run on a hosted runner; Linux/WSL validation was deferred. Building Release afterward replaces the shared APK output, not the developer app already installed on the device.

Reference sandbox scenes stay excluded from application targets and automatic asset copying. Models, textures, fonts and the configured runtime scene must be manually selected for optional bundling or supplied separately. Do not commit generated files or binary assets. New implemented contracts belong in `ARCHITECTURE.md`; deferred work and limitations belong in `ROADMAP.md`.
