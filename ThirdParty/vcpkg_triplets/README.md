# Custom vcpkg triplets

All project-owned overlay triplets live here. CMake and the Android dependency helper use this directory; installed packages default to `ThirdParty/vcpkg_installed`.

- `x64-windows.cmake`: dynamic libraries and CRT, with GLAD's Core OpenGL profile.
- `x64-linux.cmake`: static libraries, Linux system selection and GLAD's Core profile; disables Assimp's unused 3MF exporter to prevent kubazip/libzip symbol collisions.
- `arm64-android.cmake`: inherits the pinned vcpkg Android triplet and disables the same unused Assimp exporter.

Preserve these files and `ThirdParty/vcpkg_installed` when cleaning build artifacts. Do not edit the `ThirdParty/vcpkg` submodule's built-in triplets.
