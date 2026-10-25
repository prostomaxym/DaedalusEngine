# 🎯 Daedalus Engine Architecture Improvement Plan

## Understanding
Review of the engine architecture (Core, Renderer, Platform, Events, ECS, Physics, Threads, Debug, Config) to identify design issues and propose an ordered refactoring plan. Findings come from reading Application, Renderer, EventEngine and the module/CMake file lists; some items (marked "verify") need confirmation while implementing.

## Assumptions
- Quality and maintainability take priority over speed; changes are incremental and keep the Sandbox and tests building on Windows and Linux.
- C++17 stays; no new dependencies.

## Design issues found
1. Global lifecycle and static singletons. `Application::GetInstance()` lazily does `new Application()` (a non-pure class with a client-defined subclass makes this wrong), and the constructor also sets `s_instance`. Renderer, Physics, ResourceManager, Input, Log and Config are all static classes with hidden init order. The destructor tears things down manually, in an order that is not tied to construction (Log is shut down inside Application).
2. Application is a god object and the constructor does too much. It loads configs, creates the window, starts threads, initializes the renderer, physics and resources, and builds debug UI. It cannot be constructed headless, so it is not testable.
3. Renderer is a static class with a global `s_data`. Its header includes every pass, resource and primitive header, which gives heavy compile coupling. `DebugInfo` (debug textures) is baked into the public renderer API. The renderer also holds raw non-owning pointers to lights and models (`std::vector<LightSource*>`) with no lifetime guarantee. The pipeline is hard-wired (shadow, geometry, SSAO, light), with no render-graph or pass abstraction.
4. Renderer API abstraction is leaky. `Renderer` uses raw GL ids (`uint32_t id_in`, `ComputeTexture`), and the `Platform/OpenGL` implementations sit beside the windowing code. API-specific details such as uniform and storage buffers leak through `Renderer`. `Platform` mixes OS (Windows), windowing (GLFW) and graphics (OpenGL) concerns.
5. The event system is fragile. `EventEngine` is a `ThreadRunner` with a `std::deque` that `AddEvent` modifies, with no visible synchronization in the header even though mutex and condition_variable are included (verify). Events raised from the GLFW callback are queued to another thread, but `Application::OnEvent` runs layers on it. This is a likely data race, and event handling order is not tied to the frame. `OnWindowResized` returns true (consumed) so layers never see resizes. Events are logged at trace level through formatting on each event.
6. The main loop is minimal and unstructured. There is no fixed-timestep update versus render split, no separate `OnRender` or `OnImGui` phase for layers, and `Physics` is not stepped in `Run` (verify). FPS limiting uses sleep. Timers are created in `Run`, with no engine-wide time service.
7. Module dependency direction is unclear. `Core/Application.h` includes Debug/ImGuiLayer, Events and Window. `ECS` RenderSystem depends on the static Renderer. `Macros.h` and `dlpch.h` sit at the engine root, outside any module. Public headers pull in heavy includes, so there is no clean public/private boundary.
8. Ownership and API inconsistencies. Raw `new ImGuiLayer` is passed to a unique_ptr and also kept as a raw pointer. `GetMainScene()` goes through the layer stack and is marked "debugging only". `Platform::createWindow` has different casing from the rest of the API. `Application(const Application&&)` is declared deleted with a const rvalue, which is wrong. `DAEDALUS_API` is placed on classes that contain STL members (MSVC C4251 risk), and static data on exported classes is fragile for DLL builds.
9. Resource management. `ResourceManager` is a static global with no handles, no ref-counting or caching policy, and no async loading even though `ThreadPool` exists. Shaders are loaded from a path at startup.
10. Config is static (`GraphicsConfig::GetX()`) with INI reads scattered across call sites, and there is no central settings object that can be injected into tests.
11. Testing and build. Tests cover mostly Threads/Events/Utils; anything touching a GL context is untested. The explicit file lists are fine, but the Renderer, Platform and Engine CMake files are separate sub-lists, which makes it easy to forget entries. There is no layering of targets (core, renderer, platform), so everything lives in one engine library.
12. Error handling and assertions. Assertion and logging mechanisms are mixed (Macros.h and Log), and `Log::Shutdown` is called from Application.

## Approach
Do the work in small, independently buildable steps. First fix correctness bugs (events threading, deleted move constructor, singleton creation). Then make lifetimes explicit by turning the static modules into instance-owned subsystems owned by an `Engine`/`Application` context. Then decouple the renderer behind a narrow interface, and only after that split targets and extend tests. Each step ends with a build and the gtest run. For design choices that change public API shape (steps 4, 6, 7), propose options and get approval before implementing.

## Key Files
- source/dlEngine/Core/Application.h/.cpp - lifecycle, loop, singleton
- source/dlEngine/Events/EventsEngine.h/.cpp - threading model for events
- source/dlEngine/Renderer/API/Renderer.h - static renderer and heavy includes
- source/dlEngine/Platform/* - mixed OS/window/graphics code
- source/dlEngine/Core/ResourceManager.h, Config/*, Physics/Physics.h
- source/dlEngine/CMakeLists.txt and the Platform/Renderer CMakeLists.txt
- source/dlUnitTests/CMakeLists.txt

## Risks & Open Questions
- Do you want Renderer to stay a static facade or become an owned instance? This is the biggest API change.
- Is multi-threaded event handling intentional (future job system), or should events be dispatched on the main thread?
- A single-library layout versus several targets (Core, Renderer, Platform) affects build times and DLL export handling.
- Findings marked "verify" were inferred from headers only.

**Last Updated**: 2026-10-25 06:13:47

## 📝 Plan Steps
-  **Verify the suspect areas by reading EventsEngine.cpp, Physics usage in the loop, and CMake targets, and record confirmed findings**
-  **Fix correctness bugs: the deleted move constructor signature, lazy `new Application` in GetInstance, and raw ImGuiLayer ownership**
-  **Make event handling safe: synchronize the queue, or dispatch events on the main thread once per frame before layer updates; let layers see resize events**
-  **Propose and agree on the subsystem ownership design (Engine context owning Window, Renderer, Physics, ResourceManager, Config)**
-  **Split the Application constructor into init phases and move shutdown order into explicit subsystem destructors; remove Log::Shutdown from Application**
-  **Convert Renderer from static globals to an owned instance (or a clearly scoped facade) and remove raw GL ids from its public API**
-  **Reduce Renderer.h includes using forward declarations and move DebugInfo into a debug-only interface**
-  **Introduce a render pass interface so that passes are composable rather than hard-wired**
-  **Separate Platform into OS, Windowing (GLFW) and Graphics (OpenGL) folders, and move OpenGL implementations under Renderer/API/OpenGL**
-  **Extend the layer interface with OnRender and OnImGui, and add a fixed-timestep update with physics stepping**
-  **Replace static Config with an injectable Settings object loaded once**
-  **Add handle-based resource caching, and optional async loading through ThreadPool**
-  **Split the CMake into core, renderer and platform targets with a clear dependency direction**
-  **Add gtest coverage for LayerStack, EventEngine, Config and Application headless init; run the full build and tests on Windows and Linux CI**

