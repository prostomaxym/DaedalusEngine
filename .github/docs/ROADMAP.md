# Daedalus Engine – Audit & Roadmap

Status legend: 🟢 solid · 🟡 functional, needs work · 🔴 missing / stub

## 1. Audit of existing systems

| Module | State | What exists | Gaps / issues found |
|---|---|---|---|
| Build / CI | 🟡 | CMake + vcpkg manifest (C++20), per-module static libs → engine DLL, Windows & Linux CI (dev/master/release) running `ctest`; separate Windows Release/ReleaseDevTools install and packaging helpers with explicit configuration selection and configuration-matched DLLs | No sanitizers/warnings-as-errors/clang-tidy/format check in CI. |
| Core | 🟡 | `Application` loop, `Layer` stack, `Input` (keyboard/gamepad enable toggles), fixed-step clock, entry point owns app via `unique_ptr` | No engine
| Config | 🟡 | `GraphicsConfig`, `KeybindConfig`, `PathConfig` (ini, simpleini); INI-relative folders and portable relative saves with legacy absolute-path migration | No runtime reload, no validation/defaults reporting, no CVar-style console binding. Paths on different Windows drives cannot be saved relatively. |
No profiler/instrumentation.
| Events | 🟢 | Application/Key/Mouse/Gamepad events, thread-safe queue with owner-thread dispatch, MT tests | No custom/game event types, no event priorities or subscriber handles. |
| Threads | 🟡 | `ThreadPool` (task exceptions logged, worker survives), `ThreadRunner`, `ThreadJoiner` | No job system
| Utils | 🟡 | Timer, DeltaTime, FPSLocker, RingVector, Random, Utf8, WorkingDirectory | `WorkingDirectory` hardcodes `bin`, `path.ini`, 5-level upward search. No virtual file system. |
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
1. **Profiler**:
2. **Job system**: task dependencies/counters on top of ThreadPool, main-thread affinity tasks, parallel-for; used by asset loading and ECS systems.
3. **Virtual file system & asset pipeline**: mount points from `PathConfig`, asset IDs/UUIDs, async loading via jobs, hot-reload (shaders first), asset registry/metadata.
4. ✅ Console `version` command reports application/build versions from the existing `version.txt` major/minor/patch fields and compiled configuration/platform/architecture/compiler/developer-tools status, with graphics-independent regression coverage and shared packaging version metadata. ⬜ **Debug draw API** + **CVar/console** system binding Config values at runtime.
5. **Input actions**: action/axis mapping on top of `KeybindConfig`, gamepad support, rebinding.

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
