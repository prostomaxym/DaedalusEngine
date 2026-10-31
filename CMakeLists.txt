cmake_minimum_required(VERSION 3.16.3)

#------------------ Project config ------------------------------#
project(
  Daedalus
  VERSION 1.0.0
  LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeScripts/Version.cmake)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Build configurations: Debug, Release (clean production) and ReleaseDevTools (Release + dev tools)
get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (IS_MULTI_CONFIG)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release;ReleaseDevTools" CACHE STRING "Available build configurations" FORCE)
elseif (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ReleaseDevTools inherits Release flags
foreach(lang C CXX)
  set(CMAKE_${lang}_FLAGS_RELEASEDEVTOOLS "${CMAKE_${lang}_FLAGS_RELEASE}")
endforeach()
set(CMAKE_EXE_LINKER_FLAGS_RELEASEDEVTOOLS "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASEDEVTOOLS "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
set(CMAKE_STATIC_LINKER_FLAGS_RELEASEDEVTOOLS "${CMAKE_STATIC_LINKER_FLAGS_RELEASE}")
# Third-party packages only provide Debug/Release binaries
set(CMAKE_MAP_IMPORTED_CONFIG_RELEASEDEVTOOLS Release)

add_compile_definitions($<$<OR:$<CONFIG:Debug>,$<CONFIG:ReleaseDevTools>>:DL_DEV_TOOLS>)

# Platform specific config
IF(WIN32)
  set(PLATFORM_NAME "windows")
  add_compile_definitions(DL_PLATFORM_WINDOWS)
  add_compile_definitions(_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)
  add_compile_definitions(_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS)
ELSEIF(UNIX)
  set(PLATFORM_NAME "linux")
  add_compile_definitions(DL_PLATFORM_LINUX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
ENDIF()

if (MSVC)
    add_compile_options(/utf-8)
    add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)
endif()

# Binaries output dir config
set(COMPILE_OUTPUT_FOLDER ${CMAKE_SOURCE_DIR}/runtime/bin/${PLATFORM_NAME}-x64/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_FOLDER})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_FOLDER})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_FOLDER})

# Tests must be enabled in the root directory for ctest to work from the build root
if (ENABLE_TESTS)
  enable_testing()
endif()

# Subprojects config
add_subdirectory(source)

# Install setup
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeScripts/Install.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeScripts/Pack.cmake)
