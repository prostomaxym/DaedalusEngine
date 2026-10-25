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
- `Platform/Nvidia`, `ResourcesMonitor`: need GPU/OS counters.
- `Core/Application`, `Core/Input`, `Debug/ImGuiLayer`, `ImGuiWindow`: depend on a window and the ImGui context.
- `ECS/Systems/RenderSystem`: needs the renderer. `PhysicsSystem` and `NativeScript` can be tested once Scene setup is verified.
- `Utils/FPSLocker`, `Core/ResourceManager`, `Utils/WorkingDirectory`: timing-/filesystem-dependent; candidates for later with injected clock/paths.
- `ECS/Systems/PhysicsSystem`: builds `Entity` handles against the owning `Scene` registry (private) and `Scene::OnUpdateRuntime` also runs the render system; needs the system decoupled from `Scene` to be tested in isolation.
