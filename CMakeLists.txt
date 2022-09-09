cmake_minimum_required(VERSION 3.16.3)

project(
  Daedalus
  VERSION 1.0.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(COMPILE_OUTPUT_FOLDER ${CMAKE_SOURCE_DIR}/bin/x64/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_FOLDER})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_FOLDER})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${COMPILE_OUTPUT_FOLDER})

IF(WIN32)
  add_compile_definitions(DL_PLATFORM_WINDOWS)
ELSEIF(UNIX)
  add_compile_definitions(DL_PLATFORM_UNIX)
ENDIF()

add_subdirectory(dlEngine)
add_subdirectory(dlSandbox)

if (${ENABLE_TESTS})
    enable_testing()
    add_subdirectory(ThirdParty/googletest)
    add_subdirectory(dlUnitTests)
endif()