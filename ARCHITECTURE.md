# Architecture

High-level map of the Daedalus engine. For build commands and coding conventions see [.github/copilot-instructions.md](.github/copilot-instructions.md).

## Projects

| Project | Path | Role |
|---|---|---|
| `dlEngine` | `source/dlEngine` | Engine library (exports via `DAEDALUS_API`) |
| `dlSandbox` | `source/dlSandbox` | Sample client; implements `CreateApplication()` (`SandboxApp.cpp`) |
| `dlUnitTests` | `source/dlUnitTests` | gtest suite, one executable (`-DENABLE_TESTS=ON`) |
| shaders | `source/shader` | GLSL shaders loaded at runtime |

The client links the engine; the engine's entry point (`Core/EntryPoint.h`) calls the client's `CreateApplication()`.

## Engine modules (`source/dlEngine`)

| Module | Responsibility | Key files |
|---|---|---|
| `Core` | Application, Engine ownership, layers, window and input interfaces | `Application`, `Engine`, `Layer`, `LayerStack`, `Window`, `Input`, `EntryPoint.h` |
| `Config` | INI-backed settings (SimpleIni) | `GraphicsConfig`, `KeybindConfig`, `PathConfig` |
| `Debug` | Logging with category/level filtering, ImGui debug UI | `Log`, `ImGuiLayer`, `DebugOverlay`, `ConsoleOverlay` |
| `Events` | Thread-safe enqueue, owner-thread dispatch, event types | `Event`, `EventDispatcher`, `EventsEngine`, `EventTable` |
| `Threads` | Thread helpers | `ThreadRunner`, `ThreadPool`, `ThreadJoiner` |
| `Platform` | OS and windowing backends behind abstractions | `Platform.h`, `GLFW/`, `Windows/`, `Linux/`, `OpenGL/` |
| `Renderer` | Deferred renderer, passes, GPU resources, caches and statistics | `API/`, `Objects/`, `Passes/`, `Primitives/`, `Resourses/`, `Resources/` |
| `ECS` | Scene management, EnTT entities, components and systems | `SceneManager`, `Scene`, `Entity`, `Components.h`, `NativeScript`, `Systems/` |
| `Physics` | Engine-owned world factory and independent Bullet worlds | `Physics`, `PhysicsWorld` |
| `Utils` | Small shared utilities | `Timer`, `DeltaTime`, `FPSLocker`, `RingVector`, `KeyCodes`, `WorkingDirectory` |

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

## Platform abstraction

The rest of the engine uses `Window`, `Input` and `Platform.h`. OS-specific code lives in `Platform/Windows` and `Platform/Linux`; GLFW is shared in `Platform/GLFW`; the OpenGL implementation of the renderer API is in `Platform/OpenGL`. Windows headers must not appear outside `Platform/Windows`.

## Renderer

- `Renderer` (static facade, `Renderer/API/Renderer.h`) exposes `BeginFrame(camera)`, `Submit(model, transform)`, `Draw(...)`, `FlushPipeline()`.
- `RendererAPI` / `RenderCommand` abstract the graphics API; `OpenGLRendererAPI` is the current backend.
- Passes: `ShadowPass`, `DeferredGeometryPass`, `SSAOPass`, `DeferredLightPass`.
- Scene objects: `Camera`, `LightSource` types (directional, point, spot), `Geometry` (frustum, AABB, sphere helpers), `Cubemap`.
- Primitives wrap GPU objects (`FrameBuffer`, `UniformBuffer`, `ShaderStorageBuffer`); resources are `Mesh`, `Model`, `Shader`, `ShaderLibrary`.

`Renderer/Resources/ResourceManager` is an Engine-owned instance behind a static facade. Loading/upload and cache statistics are separate translation units sharing private state. All access and GPU-resource release must occur on the graphics thread; synchronous loading is not a worker-thread API. `PurgeUnused()` releases cache-only references, parents before their dependencies, at scene transitions rather than every frame. External `shared_ptr` references keep resources alive. Shaders remain in their library until shutdown/reload.

Exported facades hide subsystem state behind private pointers and use forward declarations. `Scene` retains a visible EnTT registry for the templated `Entity` API; public STL/shared-pointer types still require matching compiler/CRT settings. This is not a stable plugin ABI. Static module targets remain linked into one engine DLL; whole-archive linking preserves standalone public APIs on both MSVC and Linux.

## ECS

`Scene` wraps an `entt::registry`; `Entity` is a thin handle. Components are in `Components.h` and `SceneComponents.inl`. `NativeScript` and `NativeScriptComponent` attach C++ behavior to entities. `Systems/RenderSystem` and `Systems/PhysicsSystem` bridge ECS data to the renderer and Bullet.

Each PhysicsSystem owns an independent PhysicsWorld. `Physics::CreateWorld()` uses the Engine-owned factory's default gravity; `scene.GetPhysicsWorld()` exposes scene-local operations. Body IDs are validated by their owning world and are never pointer casts. Components retain exclusive cleanup responsibility using weak world references, so moving/detaching a component or destroying the scene cannot leave a cleanup callback dereferencing a dead world. Runtime stop releases component bodies; restart recreates them from resolved creation descriptions. Bullet alone resolves contacts; collision notifications are delivered after manifold traversal and can remove bodies safely.

Scenes can be loaded from versioned JSON files through `SceneSerializer`. Persistent records and the JSON codec are separate from runtime resources. See [Scene serialization](docs/SceneSerialization.md) for the file format, native-script registration, client workflow, and supported scope.

## Tests

`source/dlUnitTests/<Module>Tests/` mirrors engine modules (Config, Core, Debug, ECS, Events, Physics, Renderer, Threads, Utils). Every test file is listed in `source/dlUnitTests/CMakeLists.txt` and built into a single `dlUnitTests` executable registered with `ctest`. Tests must not need a GL context.
