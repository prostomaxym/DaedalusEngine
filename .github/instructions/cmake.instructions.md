---
applyTo: "**/CMakeLists.txt,**/*.cmake"
---
# CMake rules for Daedalus
- Keep explicit source file lists (no `file(GLOB)`); keep `source_group(TREE ...)` for IDE layout.
- Use target-based commands (`target_link_libraries`, `target_include_directories`, `target_compile_definitions`) with correct PUBLIC/PRIVATE scope. Avoid directory-wide flags except in the root file.
- Dependencies come from vcpkg via `find_package(... CONFIG REQUIRED)`; add them to `vcpkg.json` too.
- Must work on Windows (MSVC, Visual Studio generator) and Linux (GCC/Clang, Ninja/Make).
- Output goes to `runtime/bin/<platform>-x64/`; do not change this.
