cmake_minimum_required(VERSION 3.16.3)

project(
  Daedalus
  VERSION 1.0.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

IF(WIN32)
  add_compile_definitions(DL_PLATFORM_WINDOWS)
ELSEIF(UNIX)
  add_compile_definitions(DL_PLATFORM_LINUX)
ENDIF()

IF(WIN32)
  set(PLATFORM_NAME "windows")
ELSEIF(UNIX)
  set(PLATFORM_NAME "linux")
ENDIF()

set(COMPILE_OUTPUT_FOLDER ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}-x64/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_FOLDER})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_FOLDER})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_FOLDER})

add_subdirectory(dlEngine)
add_subdirectory(dlSandbox)

if (${ENABLE_TESTS})
    enable_testing()
    add_subdirectory(ThirdParty/googletest)
    add_subdirectory(dlUnitTests)
endif()