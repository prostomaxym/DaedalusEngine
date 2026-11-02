set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(GLAD_PROFILE core)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

if(PORT STREQUAL "assimp")
	# The unused 3MF exporter pulls in kubazip, whose C symbols collide with libzip.
	list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS -DASSIMP_BUILD_3MF_EXPORTER=OFF)
endif()

