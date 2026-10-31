# Architecture

High-level map of the current Daedalus implementation. For build commands and coding conventions see [Copilot instructions](../copilot-instructions.md); planned work and audit status belong in [ROADMAP.md](ROADMAP.md). Source and runtime paths below are relative to the repository root.

## Projects

| Project | Path | Role |
|---|---|---|
| `dlEngine` | `source/dlEngine` | Shared engine library, assembled from static module libraries (exports via `DAEDALUS_API`) |
| `dlSandbox` | `source/dlSandbox` | Client executable; implements `CreateApplication()` (`SandboxApp.cpp`) and loads a configured runtime scene |
| `dlUnitTests` | `source/dlUnitTests` | gtest suite, one executable (`-DENABLE_TESTS=ON`) |
| shaders | `source/shader` | GLSL shaders loaded at runtime |

The client links the engine; `Core/EntryPoint.h` supplies the executable entry point and calls the client's `CreateApplication()`.

The root CMake project requires C++20 and supports Windows and Linux. Each engine module is a static library linked privately into `dlEngine`; whole-archive linking on MSVC and Linux retains public APIs without internal callers. This is one shared engine library, not independently loaded module plugins. Debug and ReleaseDevTools enable `DL_DEV_TOOLS`; Release is the production configuration. Binaries are emitted under `runtime/bin/<platform>-x64`, and build steps copy shaders to `runtime/shader`. Source files are registered explicitly in each target's `CMakeLists.txt`.

## Engine modules (`source/dlEngine`)

| Module | Responsibility | Key files |
|---|---|---|
| `Core` | Application, Engine ownership, layers, window and input interfaces | `Application`, `Engine`, `Layer`, `LayerStack`, `Window`, `Input`, `EntryPoint.h` |
| `Config` | INI-backed settings (SimpleIni) | `GraphicsConfig`, `KeybindConfig`, `PathConfig` |
| `Debug` | Logging with category/level filtering, ImGui debug UI and build-information reporting | `Log`, `ImGuiLayer`, `DebugOverlay`, `ConsoleOverlay`, `BuildInfo` |
| `Events` | Thread-safe enqueue, owner-thread dispatch, event types | `Event`, `EventDispatcher`, `EventsEngine`, `EventTable` |
| `Threads` | Worker pool, task futures and thread-lifetime helpers | `ThreadRunner`, `ThreadPool`, `ThreadJoiner`, `DaedalusThreads` |
| `Platform` | OS and windowing backends behind abstractions | `Platform.h`, `GLFW/`, `Windows/`, `Linux/`, `OpenGL/` |
| `Renderer` | Deferred renderer, passes, GPU resources, caches and statistics | `API/`, `Objects/`, `Passes/`, `Primitives/`, `Resources/` |
| `ECS` | Scene management, EnTT entities, native scripts, systems and scene persistence | `SceneManager`, `Scene`, `Entity`, `Components.h`, `NativeScriptComponent`, `NativeScripts/`, `Systems/`, `Serialization/` |
| `Physics` | Engine-owned world factory and independent Bullet worlds | `Physics`, `PhysicsWorld` |
| `Utils` | Small shared utilities | `Timer`, `DeltaTime`, `FixedStepClock`, `FPSLocker`, `RingVector`, `UUID`, `KeyCodes`, `WorkingDirectory` |

The developer console supports `clear`, `quit` and `version`, including tab completion. `version` prints application version, build version, build configuration, target platform/architecture, compiler/version and developer-tools status through `Debug/GetBuildInfo()`. Application and build versions both use the existing `major.minor.patch` value from `version.txt`, without a separate build counter. The helper needs no window or graphics context and formats its report only when requested. `version.txt` is parsed once by `CMakeScripts/Version.cmake` for both packaging and embedded application metadata; changing it triggers CMake regeneration. Build metadata describes the compiled binary, not the host running it, and excludes timestamps and Git revisions.

## Application lifecycle

`Application` owns one `Engine`. Engine owns concrete configuration, input, renderer, resource-manager, physics-factory, event-queue and scene-manager instances. Static facades such as `Renderer::` and `Input::` refer to these instances without owning them; calls outside their lifetime fail fast. Only one active application/service set is supported. Logging remains a bootstrap service available before Engine construction and after Engine shutdown.

Construction order:
1. Install platform failure handling and create Engine's configuration instances; load configs.
2. Create input and the window/context from `GraphicsConfig` via `Platform`.
3. Construct the resource manager before the renderer (renderer initialization can load resources), then load shaders.
4. Start the event queue and register a window callback that clones payloads into it.
5. Attach the ImGui overlay; dev builds also attach debug and console windows. Client layers may request a loaded scene for activation on the next frame boundary.

Main loop (`Application::Run`):
1. Poll window events, update input, and dispatch the queued event snapshot on the application thread.
2. Commit a pending scene switch; purge resources no longer externally referenced.
3. Run fixed scene/physics and layer updates at approximately 60 Hz (`FixedStepClock`). Catch-up is limited to eight ticks per frame; excess elapsed time after a stall is deliberately dropped.
4. Run variable layer updates and scene scripts, then scene/layer rendering.
5. Begin one ImGui frame, invoke layer UI hooks, and render ImGui once.
6. Swap buffers and apply frame limiting via `FPSLocker`.

Shutdown rejects queued events and destroys managed scenes while client layers still exist. Layers detach next; ImGui saves its config before destroying its context. Engine then destroys systems in reverse ownership order, preserving the graphics context until renderer and cache destruction finish. Its configuration session saves ordinary settings last. `Application::GetInstance()` is cleared after Engine destruction.

## Layers

A `Layer` receives fixed-update, update, render, ImGui and event hooks; it does not implicitly own a Scene. `LayerStack` keeps layers below overlays. Events are offered top-down until handled. `Engine::GetScenes()` owns the active scene; `Application::GetMainScene()` is a non-owning debug accessor, independent of layer ordering. Scene requests are committed between event dispatch and simulation, never implicitly during script execution.

## Events and threading

- `Event` subclasses carry a category and type (window, key, mouse, gamepad).
- `EventDispatcher::ProcessEvent<T>` invokes a handler only when the event type matches.
- Any producer thread may enqueue an owned event. `EventEngine::DispatchPending()` runs only on the thread that constructed the queue, without holding the producer mutex during callbacks.
- Each dispatch drains a snapshot. Handler-produced events wait until the next frame, bounding recursive work. Stop rejects new events and discards queued work; producers must finish before the queue object is destroyed.
- Default event lifetime is unbounded; explicit zero TTL expires immediately. Pause holds queued events and resume restarts their TTL timers.
- Window callbacks clone payloads because their original stack objects expire before dispatch. Custom event types use `EVENT_CLASS_TYPE` (or implement `Clone()` themselves); event payloads must support copying or supply an appropriate clone implementation.
- `Application::OnEvent` handles close/resize, then forwards unhandled events to layers. Main-thread scene, layer and graphics access must not be performed by producer threads.

`ThreadPool` runs queued tasks on a fixed set of workers and can return futures; `ThreadJoiner` joins workers before shared state is destroyed. Pool shutdown stops workers rather than draining all queued tasks. `ThreadRunner` is a separate reusable start/stop helper, not the application loop or event dispatcher. Model import uses the shared `DaedalusThreads` pool to read/decode texture files, waits for those futures, then creates GPU textures on the calling graphics thread. There is no general asynchronous scene-loading or multithreaded ECS/render scheduler.

## Platform abstraction

The rest of the engine uses `Window`, `Input` and `Platform.h`. OS-specific code lives in `Platform/Windows` and `Platform/Linux`; GLFW is shared in `Platform/GLFW`; the OpenGL implementation of the renderer API is in `Platform/OpenGL`. Windows headers must not appear outside `Platform/Windows`.

## Renderer

- `Renderer` (static facade, `Renderer/API/Renderer.h`) exposes `BeginFrame(camera)`, `Submit(model, transform)`, `Draw(...)`, `FlushPipeline()`.
- `RendererAPI` / `RenderCommand` abstract the graphics API; `OpenGLRendererAPI` is the current backend.
- `FlushPipeline()` executes `ShadowPass` → `DeferredGeometryPass` → `SSAOPass` → `DeferredLightPass`. A stack-local `FrameContext` carries submitted models, camera/frustum, viewport and texture outputs between passes. This is an explicitly ordered pipeline, not a render graph.
- After the deferred passes, G-buffer depth is copied to the default framebuffer. `ECS/Systems/RenderSystem` then draws scene skyboxes directly through `Renderer::Draw`; skyboxes are not part of the pass vector.
- Scene objects: `Camera`, `LightSource` types (directional, point, spot), `Geometry` (frustum, AABB, sphere helpers), `Cubemap`.
- Primitives wrap GPU objects (`Framebuffer`, vertex/index buffers, vertex arrays, `UniformBuffer`, `ShaderStorageBuffer`); resources include `Mesh`, `Model`, `Material`, `Texture`, `Shader` and `ShaderLibrary`.

`Renderer/Resources/ResourceManager` is an Engine-owned instance behind a static facade. Loading/upload and cache statistics are separate translation units sharing private state. All access and GPU-resource release must occur on the graphics thread; synchronous loading is not a worker-thread API. `PurgeUnused()` releases cache-only references, parents before their dependencies, at scene transitions rather than every frame. External `shared_ptr` references keep resources alive. Shaders remain in their library until shutdown/reload.

Exported facades hide subsystem state behind private pointers and use forward declarations. `Scene` retains a visible EnTT registry for the templated `Entity` API; public STL/shared-pointer types still require matching compiler/CRT settings. This is not a stable plugin ABI.

## ECS

`Scene` wraps an `entt::registry` and a UUID-to-entity map; `Entity` is a non-owning handle. Components are in `Components.h` and `SceneComponents.inl`. Newly created entities receive ID, tag and transform components. Scenes retain their name and primary-camera UUID; programmatically authored scenes can also resolve the legacy `Main Camera` name when no primary UUID is set.

`SceneManager` owns one active scene and one pending request on its owning application thread. A newer request replaces the pending request. At the frame boundary, it stops the old scene, transfers ownership and optionally starts the new scene; a null request unloads the scene. Failed runtime startup is logged and leaves the new scene active but stopped. Runtime startup requires a camera and an application/window. Pause suspends scripts and physics but still permits rendering.

`NativeScriptComponent` owns C++ script instances keyed by type. Scripts receive variable-step `OnUpdate`; the engine includes `NativeScripts/CameraController`, while clients supply their own scripts. `Systems/RenderSystem` collects lights, submits models and draws skyboxes. `Systems/PhysicsSystem` advances the scene's Bullet world during fixed updates.

Each PhysicsSystem owns an independent PhysicsWorld. `Physics::CreateWorld()` uses the Engine-owned factory's default gravity; `scene.GetPhysicsWorld()` exposes scene-local operations. Body IDs are validated by their owning world and are never pointer casts. Components retain exclusive cleanup responsibility using weak world references, so moving/detaching a component or destroying the scene cannot leave a cleanup callback dereferencing a dead world. Runtime stop releases component bodies; restart recreates them from resolved creation descriptions. Bullet alone resolves contacts; collision notifications are delivered after manifold traversal and can remove bodies safely.

### Scene persistence

`ECS/Serialization` separates authored data from runtime entities, GPU resources, physics handles and script instances:

| Type | Responsibility |
|---|---|
| `SceneRecord` | Versioned persistent scene/component records, stable entity UUIDs and asset-relative paths |
| `SceneJson` | Strict JSON parsing, validation, serialization and record file I/O, independent of graphics services |
| `ScriptRegistry` | Client-registered stable script names and property adapters for instantiation/capture |
| `SceneSerializer` | Instantiate/capture runtime scenes and combine those operations with file load/save |

Version 1 supports transforms, models, cameras, directional/point/spot lights, skyboxes, rigid/kinematic bodies and registered native scripts. The JSON implementation uses `nlohmann-json` privately in ECS; JSON library types are not exposed to clients. Instantiation/loading needs an empty, stopped scene and runs on the graphics thread after engine services exist; it does not start runtime. Capture requires a stopped scene. APIs return success/failure with an error diagnostic. See [Scene serialization](SceneSerialization.md) for the complete schema, validation rules and supported scope.

## Runtime assets and sandbox

`dlSandbox/ExampleLayer::OnAttach` registers client script adapters, loads a scene using `SceneSerializer::Load(PathConfig::GetScenePath(), ..., PathConfig::GetAssetsPath(), ...)`, and requests activation through `SceneManager`. There is no compiled-in example-scene fallback; registration or loading failure is logged.

`PathConfig` resolves `AssetsFolder`, `ShadersFolder`, `LogsFolder` and `ConfigsFolder` relative to the directory containing `path.ini`, including defaults when loading a missing file. Runtime getters return resolved absolute paths. Saving writes these folders relative to the destination INI directory, using forward slashes and lexical paths without requiring assets to exist. Legacy absolute entries remain loadable and are converted on save; paths on incompatible filesystem roots (such as different Windows drives) remain absolute with a warning. Explicit `LoadDefault()` uses the current working directory.

`PathConfig` resolves `[Path] SceneFile` relative to `AssetsFolder`; `GraphicsConfigFile` and `KeybindConfigFile` are relative to `ConfigsFolder`. Saving preserves these bases, converting legacy absolute filenames when possible. The default startup file is `scenes/startup.scene.json` under the runtime asset root. Scene JSON, models, textures and cubemap faces are supplied manually. Files in `source/dlSandbox/scenes` are reference examples only, excluded from the sandbox target and automatic copying. Changing the configured startup scene does not require rebuilding the client. Installation packages the manually supplied runtime assets, not example scenes directly from source.

## Tests

`source/dlUnitTests/<Module>Tests/` mirrors engine modules (Config, Core, Debug, ECS, Events, Physics, Renderer, Threads, Utils). Every test file is listed in `source/dlUnitTests/CMakeLists.txt` and built into a single `dlUnitTests` executable registered with `ctest`. Tests must not need a GL context.
