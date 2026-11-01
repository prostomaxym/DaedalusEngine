# Daedalus Engine – Audit & Roadmap

Status legend: 🟢 solid · 🟡 functional, needs work · 🔴 missing / stub

## 1. Audit of existing systems

| Module | State | What exists | Gaps / issues found |
|---|---|---|---|
| Build / CI | 🟡 | CMake + vcpkg manifest (C++20), per-module static libs → engine DLL, Windows & Linux CI (dev/master/release) running `ctest`; separate Windows Release/ReleaseDevTools install and packaging helpers with explicit configuration selection and configuration-matched DLLs | No sanitizers/warnings-as-errors/clang-tidy/format check in CI. |
| Core | 🟡 | `Application` loop, `Layer` stack, `Input` (keyboard/gamepad enable toggles), fixed-step clock, entry point owns app via `unique_ptr` | No engine
| Config | 🟡 | `GraphicsConfig`, `KeybindConfig`, `PathConfig` (ini, simpleini); INI-relative folders and portable relative saves; typed console CVars with validated live FPS/keybind setters and read-only startup settings | No runtime INI reload or general load-time validation/defaults reporting. Startup-only GPU/window/mount values are intentionally read-only. Paths on different Windows drives cannot be saved relatively. |
| Debug | 🟢 | Initial CPU/GPU timing profiler, tab-local debug UI, build report, runtime CVar console commands; Renderer supplies developer world-space line/box/sphere drawing | Optional profiler enhancements are tracked separately below; allocation tracking is not implemented. |
| Events | 🟢 | Application/Key/Mouse/Gamepad events, thread-safe queue with owner-thread dispatch, MT tests | No custom/game event types, no event priorities or subscriber handles. |
| Threads | 🟢 | Engine-owned `JobSystem`: dependency/group counters, main-thread jobs, bounded parallel-for, draining shutdown; `ThreadPool`, `ThreadRunner`, `ThreadJoiner` | Job nodes allocate on submission; no pooled job arena or automatic arbitrary-ECS scheduling. |
| Utils | 🟡 | Timer, DeltaTime, FPSLocker, RingVector, Random, Utf8, WorkingDirectory; directory/ZIP `VirtualFileSystem`, bounded reads, named UTF-8 mounts, containment checks and content fingerprints | `WorkingDirectory` hardcodes `bin`, `path.ini`, 5-level upward search. ZIP encryption and an engine archive-packing tool are not provided. |
| Platform | 🟡 | GLFW window, OpenGL context, Win/Linux resources monitor (guarded `/proc` access), Nvidia high-perf request | Imprecise Linux sleep (`Platform.cpp`). |
Spot light doesn't support ortho camera. No render graph,
| `SkyboxComponent` export TODO (still open). No transform
| Physics | 🟡 | Bullet3 world, rigid bodies via `PhysicsSystem`, fixed step | Verify/extend: collision events → Events, raycasts/queries API, character controller, triggers, debug draw, serialization of all collider shapes. |
| Not yet config-driven generic `dlGame`. |
| Events, Threads, Utils, ECS/scene fixtures (own copies in `dlUnitTests/fixtures/scenes`) | No tests for
| Audio, Animation, AI, Networking, Scripting, Editor | 🔴 | – | Not started. |

## 2. Roadmap

Phases are ordered by dependency; each phase should leave the engine shippable and green on both CI platforms.

### Phase 0 – Hygiene & foundations (short)
1. ✅ C++ standard aligned on C++20 (CMake + instructions). ✅ CI runs `ctest` on both OSes. ⬜ Warnings-as-errors per target, ASan/UBSan Linux job, `.clang-format` check.
2. ✅ Merged `Renderer/Resourses` into `Renderer/Resources` (includes & CMake updated).
3. ✅ Fixed: Sandbox app ownership (`CreateApplication` → `unique_ptr` in entry point), `/proc` file guards, stale Linux TODOs, ThreadPool task exception handling, `NativeScriptComponent` → `unique_ptr` (move-only exported type), `Input::EnableKeyboardInput` now gates key queries. ⬜ `SkyboxComponent` `DAEDALUS_API` export investigation.
4. ✅ Test fixtures decoupled from `dlSandbox` (`dlUnitTests/fixtures/scenes`).
5. ✅ `path.ini` saves relative folder paths anchored to its directory; regression coverage for legacy absolute migration, missing-file defaults and relocation.

### Phase 1 – Core infrastructure
1. ✅ **Initial profiler implementation complete**: CPU scopes (`DL_PROFILE_SCOPE`), Chrome trace capture, Profiler tab, per-scope rolling stats, non-blocking OpenGL GPU timestamp queries (`GPUTimer`) covering render+ImGui, each pass, skybox, depth copy and debug compute; p95/p99 per scope; stats aggregated independently of the UI; GPU timestamps calibrated to the profiler clock; dropped GPU frames counted; sortable tables, Ctrl-to-pause. Optional enhancements below are not completion requirements.
2. ✅ **Job system**: engine-owned scheduler over ThreadPool with dependency/group counters, main-thread affinity tasks and parallel-for. Model material loading uses worker image decoding with ordered dependent main-thread uploads; RenderSystem prepares transform matrices from reusable snapshots in parallel for batches of 128+ objects. Shutdown rejects new submissions and drains accepted jobs before resource teardown. Graphics-independent regression coverage covers scheduler contracts, image decoding and transform preparation. General async scene loading and automatic arbitrary-ECS scheduling are not implemented; VFS/async asset APIs are supplied by item 3. Job submissions use bounded allocations, not a pooled job arena.
	- Validation: Windows Release and ReleaseDevTools builds and full CTest suites passed; all 26 new tests passed across 20 repetitions. Linux/WSL validation was deferred by user choice; no graphics-context smoke test was performed.
3. ✅ **Virtual file system & asset pipeline**: Engine-owned `AssetPipeline`; `assets:/`, `shaders:/`, `config:/`, `logs:/` mounts from `PathConfig`; persistent nonzero 64-bit UUID sidecars and registry metadata; job-based binary/source reads with main-thread completion; serialized worker polling for asset discovery and shader-first hot-reload. Shader includes expand recursively on workers; reload preserves shared Shader objects and the last-good program, transfers compatible uniform/buffer bindings, rejects superseded generations and refreshes cache/statistics only after success. Invalid/duplicate metadata is rejected and reported. Graphics-independent regression coverage covers mount containment, metadata persistence/relocation, async contracts, changes, includes and shutdown.
	- Validation: Windows Release and ReleaseDevTools builds and full CTest suites passed. The 26 new asset tests completed 20 repetitions in each configuration: 24 passed and 2 symlink tests skipped per run because Windows symlink creation was unavailable. Linux/WSL validation was deferred by user choice; real GL reload smoke validation has not been performed.
	- ✅ Follow-up gaps implemented: content fingerprints detect same-stamp/same-size edits; bounded read-only ZIP mounts use explicit libzip core; typed CPU model/texture imports support VFS Assimp dependencies and embedded textures; owner-thread uploads and automatic model/2D/cubemap reload preserve shared resource identity. Renderer and scene APIs accept virtual paths/UUIDs, and the sandbox uses the configured virtual scene path. Startup shader caches validate expanded-source fingerprints; ZIP shaders compile without writing archive caches. Scene reload is explicit, and initial compiler failures retain their fatal behavior as requested.
	- Follow-up validation: Windows Release and ReleaseDevTools builds and full CTest suites passed. All 24 new graphics-independent tests passed 20 repetitions in each configuration with no skips. The workspace build and final whitespace check passed. Test Explorer discovered no tests, so validation used CTest and direct GoogleTest runs. Linux/WSL was deferred by user choice; real GL upload/reload smoke testing has not been performed.
	- Limits: noncryptographic fingerprints can collide; scanning reads full source contents and ZIP repacks may reload unchanged entries. Archives require bundled UUID sidecars and exclude encryption. Model-derived colliders are not rebuilt by resource reload; explicit scene reload is required when gameplay state must be reset. GPU buffer-allocation failure paths retain existing backend behavior.
	- Runtime ZIP smoke follow-up: debugger verified that the actual Windows `Compress-Archive` ZIP opens with `ZIP_CHECKCONS` but was rejected at `fonts\\calibri.ttf`; ImGui independently used a physical ZIP-relative font path. Implemented canonical ZIP separator/index reads with collision and repack validation, detailed independent mount diagnostics, and VFS-backed retained font bytes. First Windows debug verification accepted 1,563 entries, registered all four mounts and 794 assets without metadata errors, loaded 1,647,428 font bytes into one ImGui font, and loaded `Nuke Scene`. User reported slow startup/stuttering; worker inspection identified repeated archive-directory validation in registry reads. Added synchronous scan-scoped ZIP handle/index reuse, discarded between scans (no timestamp-only cache). Post-optimization Windows debugger verification preserved 794 assets with no registry errors, confirmed scan-handle reuse and successful scene loading, and exited normally with code 0. User confirmed the issue resolved and accepted slightly slower ZIP loading than directory assets; further optimization is deferred. Nine new regressions cover Windows names, aliases, repacks including same-stamp/size changes, bounded scan reads, mount errors, UUID metadata, model dependencies and scene path/UUID loading; traversal coverage expanded. User confirmed Windows Release and ReleaseDevTools build/CTest reruns passed after the fixture correction; validation used CTest because Test Explorer discovers no tests. Linux/WSL remains deferred; standalone/model-import archive reads still reopen handles.
	- ZIP follow-up test validation: initial user-reported run and saved ReleaseDevTools CTest log showed 372 passing, two Windows symlink skips and one failing new test. The failure was a fixture-generated sidecar for a directory (`empty\\` became `empty/.meta`), not a production ZIP read failure. Corrected the test ZIP helper to generate UUID sidecars for files only; user confirmed the subsequent validation passed.
4. ✅ Console `version` command reports application/build versions from the existing `version.txt` major/minor/patch fields and compiled configuration/platform/architecture/compiler/developer-tools status, with graphics-independent regression coverage and shared packaging version metadata. ✅ **Debug draw API** + **CVar/console**: developer-only static Renderer line/axis-aligned box/wire-sphere submissions, bounded reusable depth/overlay batches, final-frame rendering with the completed scene camera and state-restoring OpenGL backend. Console `cvars [prefix]`, `get <name>`, `set <name> <value>` and CVar-name completion bind live FPS limits and existing keyboard bindings; startup-only graphics/path settings are read-only. Typed parsing and setters reject invalid input without mutation; edits use existing Config persistence on teardown.
	- Validation: workspace build and final Windows Release/ReleaseDevTools builds and full CTest suites passed. The 25 new graphics-independent regressions ran 20 repetitions per configuration: all 25 passed in Release; 24 passed and the Release-only facade test was intentionally skipped in ReleaseDevTools. Coverage includes registry registration, live reads, parsing, validation, commands, listing/completion, Config mutation/persistence, geometry, atomic capacity, storage reuse and Release no-op behavior. Test Explorer discovered no tests, so validation used CTest/direct GoogleTest. Whitespace checks passed. Linux/WSL validation is deferred by user choice; real OpenGL line/depth/state and interactive console smoke validation have not been performed.
	- Limits: debug primitives last one frame, use opaque RGB and a shared 4,096-line budget; spheres are three circles, not meshes. No text, duration, arbitrary triangles, multi-camera routing or automatic Physics debug adapter. Frames without a completed scene camera and late ImGui submissions are discarded; missing DebugDraw shader warns/discards. CVar registration snapshots existing keyboard names at console initialization; no new-key creation, gamepad rebinding, runtime INI reload or live rebuild of initialization-only GPU/window/mount settings.
	- ✅ Console completion follow-up: Tab matching was functional but did not preview suggestions while typing. Added a cached muted inline suffix for the first matching command/CVar at the end of focused input, with Tab acceptance; selections, mid-line edits, values and buffer-overflow suggestions have no ghost text. Shared graphics-independent matching retains mid-line suffix preservation. Ten new regressions cover matching, ambiguity, hint acceptance, whitespace, selections, cursor bounds and fixed-buffer capacity. Workspace and Windows Release/ReleaseDevTools builds and full CTest suites passed; all ten new tests passed 20 repetitions in each configuration without skips. Test Explorer discovered no matching tests, so validation used CTest/direct GoogleTest. Tracked/new-file whitespace checks passed. User confirmed inline completion works correctly; Linux/WSL remains deferred.
	- ✅ Debug-draw smoke follow-up: user verified debug drawing works and confirmed the debug-overlay axis issue is fixed. Removed the temporary sandbox harness, including hotkeys, per-frame submissions, placement state and diagnostic logs; the engine debug-draw API and overlay fix remain intact. Cleanup workspace and Windows Release/ReleaseDevTools builds and full CTest suites passed; whitespace checks passed. Linux/WSL remains deferred.
5. **Input actions**: action/axis mapping on top of `KeybindConfig`, gamepad support, rebinding.

6. ✅ Debug overlay split into five concrete top-level tab classes (`Debug/Tabs`), with tab-local UI state and a coordinator retaining window/input/sizing ownership.
	  - ✅ Fixed World-position axis visibility: runtime inspection found an enabled, unit-length X axis projecting to approximately 4.3 pixels under the forced 0°/0° plot view. Initialize an oblique 30°/45° view once instead of resetting rotation every frame, allowing users to orbit away from end-on or overlapping axes. Camera basis and GL-to-plot coordinate conventions are unchanged; this does not establish the tab extraction as the cause. Windows debugger relaunch verified the new rotation and an approximately 90-pixel X segment in a near-end-on camera orientation; user confirmed the UI issue resolved without new problems. Post-change Linux validation not run.

### Optional profiler technical debt
These enhancements are deferred and do not block completion of the initial CPU/GPU timing profiler.
- ⬜ **Memory profiling (optional)**: opt-in engine-managed allocation tracking, per-frame allocation counts/bytes, live/peak tracked bytes and scope attribution. Not implemented; existing RAM usage and resource-memory estimates are not allocation profiling. Coverage would exclude allocations that bypass the tracked API, third-party internals and GPU allocations.
- ⬜ Additional worker-thread (`ThreadPool`) task scopes.
- ⬜ In-overlay timeline view.
- ⬜ Capture hotkey and automatic capture on frame spikes.

### Phase 2 – Scene & gameplay foundations
1. Transform hierarchy (parent/child, dirty flags), entity UUIDs, prefabs, scene versioning/migration in serializer.
2. Physics enhancements: collision/trigger events, raycasts/shape queries, character controller, joints, debug draw, full serialization.
3. **Convert dlSandbox → dlGame**: generic app loading scenes/configs purely from runtime assets.
4. Game event types & scene-level messaging.

### Phase 3 – Rendering tech debt
Order chosen so each step unlocks the next:
1. **Render graph / pass framework** (resource lifetimes, pass ordering) – makes post-processing composable.
2. **HDR pipeline + tonemapping + exposure**, then **Bloom**, **Blur** utilities.
3. **PBR** (metal/rough, IBL from skybox, BRDF LUT); material system rework.
4. **Shadows**: robust bias calculation, CSM smoothing/blending between cascades, stable cascades, **VSM**; spot-light ortho support; **Screen-space shadows**.
5. **Transparency/blending**: sorted forward transparent pass after deferred, OIT later.
6. Culling (frustum, later occlusion), instancing/batching, **LOD**.
7. **Anti-aliasing**: FXAA baseline → **TAA** (needs motion vectors/jitter).
8. **HBAO+** (replace/extend SSAO), **SSR**, **Reflections** (probes), **Depth of field**, **Lens flares**.
9. **Particles** (GPU, compute-based), **Water**, **Procedural sky/clouds**.
10. **Global illumination** (light probes/DDGI-style), then **Ray tracing** research.

### Phase 4 – New engine systems
1. **Audio** (e.g. miniaudio/OpenAL-soft): sources/listener components, 3D spatialization, streaming, mixer buses.
2. **Animation**: skeletal animation from assimp, GPU skinning, blending/state machine, then IK.
3. **Scripting** (Lua via sol2 recommended first; Python optional): bindings for ECS/Input/Events, hot reload, sandboxing.
4. **Editor application (dlEditor)**: docking UI, scene hierarchy/inspector, ImGuizmo gizmos, asset browser, play-in-editor, undo/redo (command pattern). Depends on serialization, VFS, reflection.
5. **Reflection/metadata** for components (needed by editor, serializer, scripting) – start before editor.
6. **AI**: navmesh (Recast/Detour), pathfinding, behavior trees/state machines, perception.
7. **UI for games** (runtime UI separate from ImGui).
8. **Networking**: transport (ENet/GameNetworkingSockets), replication of ECS state, client prediction.

### Phase 5 – Render backends & platform
1. Harden `Renderer/API` abstraction (command buffers, explicit resource states) after render graph.
2. **Vulkan** backend, then **DX12** (Windows only); shader cross-compilation (glslang/SPIR-V Cross).
3. Ray tracing (Vulkan RT / DXR) on top of new backends.
4. Packaging: ✅ Windows NSIS uninstall cleanup for installation-local logs/shader caches, with default-No confirmation to delete configs and preservation of packaged settings when declined. ⬜ Asset cooking and further release build/installer improvements (extend `CMakeScripts`).

## 3. Cross-cutting rules for all phases
- Every new logic module ships with gtest coverage where no GL context is needed.
- Keep frame loop allocation-free; measure with the profiler from Phase 1 onward.
- New dependencies via `vcpkg.json` with justification.
- Both Windows and Linux CI must stay green.
- Keep this file current: when an item is implemented or a new gap is found, update the status table and mark roadmap items (✅ done, ⬜ open) in the same change.

## 4. Suggested milestone order (summary)
`M0 Hygiene` → `M1 Profiler + Jobs + VFS` → `M2 Scene/Physics + dlGame` → `M3 HDR/PBR/Shadows/Render graph` → `M4 Reflection + Editor` → `M5 Audio/Animation/Scripting` → `M6 Advanced graphics (TAA, SSR, GI, particles, water, sky)` → `M7 AI/Networking` → `M8 Vulkan/DX12/RT`.
