cmake_minimum_required(VERSION 3.16.3)

#----------------- Dependencies config --------------------------#
include(${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/vcpkg/scripts/buildsystems/vcpkg.cmake)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")

set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_triplets")


#------------------ Project config ------------------------------#
project(
  daedalus
  VERSION 1.0.0
  LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Platform specific config
IF(WIN32)
  set(PLATFORM_NAME "windows")
  add_compile_definitions(DL_PLATFORM_WINDOWS)
ELSEIF(UNIX)
  set(PLATFORM_NAME "linux")
  add_compile_definitions(DL_PLATFORM_LINUX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
ENDIF()

# Binaries output dir config
set(COMPILE_OUTPUT_FOLDER ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}-x64/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_FOLDER})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_FOLDER})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_FOLDER})

# Subprojects config
add_subdirectory(dlEngine)
add_subdirectory(dlSandbox)
 
# Tests setup
if (ENABLE_TESTS)
    enable_testing()
    add_subdirectory(dlUnitTests)
endif()


include(${CMAKE_CURRENT_SOURCE_DIR}/Install.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Pack.cmake)