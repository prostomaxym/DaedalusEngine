---
applyTo: "**/*.{h,hpp,cpp,inl}"
---
# C++ rules for Daedalus
- C++20 only. Tabs for indentation. `#pragma once`. Namespace `Daedalus`.
- Members `m_name`, statics `s_name`, locals `snake_case`, types/functions `PascalCase`.
- Use `DAEDALUS_API` on exported engine classes.
- Own with `std::unique_ptr`; raw pointers are non-owning.
- Include paths must match file-name case exactly (Linux is case-sensitive).
- No Windows/OpenGL-specific headers in platform-agnostic code; use the `Platform` and `Renderer/API` abstractions.
- Prefer forward declarations in headers to keep compile times low.
- Add every new `.h/.cpp` to the module's `CMakeLists.txt`.
