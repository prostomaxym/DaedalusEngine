# Daedalus Engine – Copilot Instructions

Personal cross-platform (Windows + Linux) C++ game engine in early development. Long-term goal: a robust, production-quality, easy-to-use engine. Quality and maintainability matter more than speed of delivery.

## Tech stack
- C++17 (`CMAKE_CXX_STANDARD 17`). Do not use C++20 features unless the standard is raised in the root `CMakeLists.txt`.
- Build system: CMake (min 3.16.3) + vcpkg manifest mode (`vcpkg.json`, vcpkg lives in `ThirdParty/vcpkg`).
- Libraries: glfw3, glad (OpenGL), imgui (+docking), imguizmo, implot/implot3d, glm, stb, assimp, entt, spdlog, fmt, simpleini, bullet3, gtest/gmock.
- Compilers: MSVC (primary, Visual Studio) and GCC/Clang on Linux (CI).
- Future: scripting language, editor application, shaders (GLSL in `source/shader`).

## Repository layout
- `source/dlEngine` – engine library. Modules: `Config`, `Core`, `Debug`, `ECS`, `Events`, `Physics`, `Platform`, `Renderer`, `Threads`, `Utils`.
- `source/dlSandbox` – sample/client application (implements `CreateApplication()`).
- `source/dlUnitTests` – gtest unit tests (built only with `-DENABLE_TESTS=ON`).
- `source/shader` – shaders.
- `CMakeScripts/` – install/pack scripts. `WinScripts/`, `LinuxScripts/` – helper scripts.
- `runtime/`, `build/`, `install/`, `vcpkg_installed/` are generated; never edit or commit them.
- `ThirdParty/` is for submodules (vcpkg); never edit.

## Build and test
Everything is configured from the repo root via CMake. All source files must be listed in the owning `CMakeLists.txt` (explicit file lists, no globbing). When adding/removing/renaming a file, update that `CMakeLists.txt`.

Windows (PowerShell, from repo root):
- Configure: `cmake -B build -DENABLE_TESTS=ON` (or `WinScripts/CMakeConfig.bat`)
- Build: `cmake --build build --config Release`
- Test: `ctest --test-dir build -C Release --output-on-failure` (or run `runtime/bin/windows-x64/dlUnitTests.exe`)

Linux:
- Configure: `LinuxScripts/CMakeConfig.sh` (uses vcpkg toolchain, x64-linux triplet)
- Build: `cmake --build build`

Always make sure changes compile on both Windows and Linux. Linux CI runs on every push; avoid MSVC-only constructs, Windows headers outside `Platform/Windows`, and case-mismatched `#include` paths (Linux is case-sensitive).

## Code conventions (match existing code)
- Indentation: tabs. Allman braces for classes/functions/namespaces as in existing files.
- `#pragma once` in headers. No `using namespace` in headers.
- Everything lives in `namespace Daedalus`.
- Types/methods: `PascalCase` (`Application::GetInstance`). Local variables and parameters: `snake_case`.
- Members: `m_` prefix (`m_window`); static members: `s_` prefix (`s_instance`).
- Exported engine classes use the `DAEDALUS_API` macro (see `Core/Macros.h`). Platform macros: `DL_PLATFORM_WINDOWS`, `DL_PLATFORM_LINUX`.
- Ownership: `std::unique_ptr` by default; raw pointers are non-owning only (comment who owns). Avoid `new`/`delete`; prefer `std::make_unique`.
- Const-correctness, `override`, `nullptr`, `enum class`, `[[nodiscard]]` where it helps.
- Special members: follow rule of 0/5; delete copy/move explicitly for singletons.
- Logging: use the engine logger (spdlog wrapper in `Debug`), not `std::cout`/`printf`.
- No exceptions for control flow in engine code; use return values/asserts as the existing code does. Check what `Debug` provides for assertions before adding new mechanisms.
- Keep the render loop allocation-free where possible; do not add per-frame heap allocations casually.
- Platform-specific code goes in `Platform/<OS>`; the rest of the engine uses abstractions (`Window`, etc.). Renderer API-specific code goes behind `Renderer/API` abstractions.
- Prefer small focused classes/files. Don't add comments that restate code; comment the "why".

## Working rules for AI changes
- Make minimal, focused changes. Do not reformat unrelated code or rename things without being asked.
- Read the surrounding module before adding code; reuse existing utilities (`Utils`, `Events`, `ECS`) instead of creating duplicates.
- New third-party dependency: add to `vcpkg.json` and link in CMake, and explain why. Prefer existing libraries.
- Add or update gtest tests in `source/dlUnitTests` for logic that can be tested without a GL context (math, containers, event system, utils, ECS logic). Register new test files in its `CMakeLists.txt`.
- After changes, build the solution and run the tests; fix warnings you introduce.
- Never commit secrets, generated folders or binary assets without being asked.
- Do not edit `ThirdParty/`, `vcpkg_installed/`, `build/`, `runtime/`.
- If a task has several valid designs (architecture, public API shape), propose the options briefly before implementing.
