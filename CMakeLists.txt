cmake_minimum_required(VERSION 3.16.3)

project(
  Daedalus
  VERSION 1.0.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

IF(WIN32)
    add_compile_definitions(DL_PLATFORM_WINDOWS)
ENDIF()

add_subdirectory(dlEngine ${binaryDir})
add_subdirectory(dlSandbox ${binaryDir})