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
| `Core` | Application singleton, layers, window and input interfaces, resource manager | `Application`, `Layer`, `LayerStack`, `Window`, `Input`, `ResourceManager`, `EntryPoint.h` |
| `Config` | INI-backed settings (SimpleIni) | `GraphicsConfig`, `KeybindConfig`, `PathConfig` |
| `Debug` | Logging with category/level filtering, ImGui debug UI | `Log`, `ImGuiLayer`, `DebugOverlay`, `ConsoleOverlay` |
| `Events` | Event types, dispatch, queue processed on a worker thread | `Event`, `EventDispatcher`, `EventsEngine`, `EventTable` |
| `Threads` | Thread helpers | `ThreadRunner`, `ThreadPool`, `ThreadJoiner` |
| `Platform` | OS and windowing backends behind abstractions | `Platform.h`, `GLFW/`, `Windows/`, `Linux/`, `OpenGL/` |
| `Renderer` | Deferred renderer, render passes, GPU resources | `API/`, `Objects/`, `Passes/`, `Primitives/`, `Resourses/` |
| `ECS` | EnTT-based scene, entities, components, systems | `Scene`, `Entity`, `Components.h`, `NativeScript`, `Systems/` |
| `Physics` | Bullet wrapper | `Physics.h` |
| `Utils` | Small shared utilities | `Timer`, `DeltaTime`, `FPSLocker`, `RingVector`, `KeyCodes`, `WorkingDirectory` |

## Application lifecycle

`Application` (`Core/Application.cpp`) is a singleton.

Construction order:
1. Install the segfault handler (`Platform`), load configs (`WorkingDirectory::LoadConfigs`).
2. Create the window from `GraphicsConfig` via `Platform::createWindow`.
3. Start the `EventEngine` thread, init the input system, register `Application::OnEvent` as the window event callback.
4. `Renderer::Init`, `Physics::Init`, `ResourceManager::Init`, load shaders from `PathConfig`.
5. Push the `ImGuiLayer` overlay; in dev builds add `DebugOverlay` and `ConsoleOverlay`.

Main loop (`Application::Run`):
1. Compute update delta time.
2. `Window::PollEvents`, `Input::Update`.
3. `Layer::OnUpdate(dt)` for every layer in stack order.
4. `Window::SwapBuffers`.
5. Frame limiting via `FPSLocker`.

Destruction saves configs and shuts down Input, Renderer, Physics, ResourceManager and Log.

## Layers

A `Layer` receives `OnUpdate` and `OnEvent`. `LayerStack` keeps layers below overlays. Events are offered to layers from the top of the stack down until one marks the event handled. A layer may own a `Scene`; `Application::GetMainScene` returns the main layer's scene.

## Events and threading

- `Event` subclasses carry a category and type (window, key, mouse, gamepad).
- `EventDispatcher::ProcessEvent<T>` invokes a handler only when the event type matches.
- `EventEngine` derives from `ThreadRunner` and owns a queue drained on its own thread. `ThreadRunner` provides Start, Stop, Pause and Resume with `OnStart`/`OnStop`/`OnResume` hooks.
- `Application::OnEvent` handles window close and resize itself, then forwards to layers.

## Platform abstraction

The rest of the engine uses `Window`, `Input` and `Platform.h`. OS-specific code lives in `Platform/Windows` and `Platform/Linux`; GLFW is shared in `Platform/GLFW`; the OpenGL implementation of the renderer API is in `Platform/OpenGL`. Windows headers must not appear outside `Platform/Windows`.

## Renderer

- `Renderer` (static facade, `Renderer/API/Renderer.h`) exposes `BeginFrame(camera)`, `Submit(model, transform)`, `Draw(...)`, `FlushPipeline()`.
- `RendererAPI` / `RenderCommand` abstract the graphics API; `OpenGLRendererAPI` is the current backend.
- Passes: `ShadowPass`, `DeferredGeometryPass`, `SSAOPass`, `DeferredLightPass`.
- Scene objects: `Camera`, `LightSource` types (directional, point, spot), `Geometry` (frustum, AABB, sphere helpers), `Cubemap`.
- Primitives wrap GPU objects (`FrameBuffer`, `UniformBuffer`, `ShaderStorageBuffer`); resources are `Mesh`, `Model`, `Shader`, `ShaderLibrary`.

## ECS

`Scene` wraps an `entt::registry`; `Entity` is a thin handle. Components are in `Components.h` and `SceneComponents.inl`. `NativeScript` and `NativeScriptComponent` attach C++ behavior to entities. `Systems/RenderSystem` and `Systems/PhysicsSystem` bridge ECS data to the renderer and Bullet.

## Tests

`source/dlUnitTests/<Module>Tests/` mirrors engine modules (Config, Core, Debug, ECS, Events, Physics, Renderer, Threads, Utils). Every test file is listed in `source/dlUnitTests/CMakeLists.txt` and built into a single `dlUnitTests` executable registered with `ctest`. Tests must not need a GL context.
