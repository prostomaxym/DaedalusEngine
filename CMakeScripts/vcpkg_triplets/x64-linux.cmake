include("${CMAKE_CURRENT_LIST_DIR}/../../ThirdParty/vcpkg_triplets/x64-linux.cmake")

if(PORT STREQUAL "assimp")
    # The unused 3MF exporter pulls in kubazip, whose C symbols collide with libzip.
    list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS -DASSIMP_BUILD_3MF_EXPORTER=OFF)
endif()
