---
applyTo: "source/dlUnitTests/**"
---
# Unit test rules
- Framework: GoogleTest/GoogleMock (from vcpkg).
- Test names: `TEST(<Subject>Test, <behavior_in_snake_case>)`; one behavior per test.
- Tests must be deterministic, fast, and must not need a window, GL context, GPU or files outside the repo.
- Floating-point comparisons: use `EXPECT_NEAR` / `EXPECT_FLOAT_EQ`, never `EXPECT_EQ`.
- Register new test files in `source/dlUnitTests/CMakeLists.txt` (`PROJECT_FILES`).
- Run: `ctest --test-dir build -C Release --output-on-failure`.

## Not testable yet (design limitations)
Revisit when the design allows; do not add tests that need a window, GL context or real hardware.
- `Platform/GLFW`, `Platform/Windows`, `Platform/Linux` windows and input: need a GLFW window/display.
- `Platform/OpenGL/*`, `Renderer/API`, `Renderer/Passes`, `Renderer/Resourses` (Shader, Texture, Mesh, Model, Material): need a GL context; no renderer abstraction can be mocked yet.
- `Platform/Nvidia` and live `ResourcesMonitor` telemetry: need GPU/OS counters. Shared byte conversions, default `GpuInfo` records and static DLL-interface linkage are headlessly testable; do not invoke telemetry or graphics queries in those tests.
- `Core/Application`, `Core/Input`, `Debug/ImGuiLayer`, `ImGuiWindow`: depend on a window and the ImGui context.
- `ECS/Systems/RenderSystem`: needs the renderer. `PhysicsSystem` and `NativeScript` can be tested once Scene setup is verified.
- `Utils/FPSLocker`, `Core/ResourceManager`: timing-/filesystem-dependent; candidates for later with injected clock/paths.
- `Utils/WorkingDirectory` startup selection is headlessly testable: inject executable/launch paths into `StartupDiscovery` and use repo-owned filesystem fixtures for selected-path/CWD/load/save behavior. Do not construct an Application or GL context for these tests.
- `ECS/Systems/PhysicsSystem`: builds `Entity` handles against the owning `Scene` registry (private) and `Scene::OnUpdateRuntime` also runs the render system; needs the system decoupled from `Scene` to be tested in isolation.
