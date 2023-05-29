cmake_minimum_required(VERSION 3.16.3)

#------------------ Project config ------------------------------#
project(
  Daedalus
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
add_subdirectory(source)

# Install setup
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeScripts/Install.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeScripts/Pack.cmake)