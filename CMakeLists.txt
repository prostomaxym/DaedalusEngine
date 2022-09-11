cmake_minimum_required(VERSION 3.16.3)

project(
  Daedalus
  VERSION 1.0.0
  LANGUAGES CXX)

# Project config
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
 
set(ENABLE_TESTS ON)
if (${ENABLE_TESTS})
    enable_testing()

    set(INSTALL_GTEST OFF)

    add_subdirectory(ThirdParty/googletest)
    add_subdirectory(dlUnitTests)
endif()

# Install config
IF(WIN32)
  install(TARGETS dlEngine 
          RUNTIME DESTINATION bin
          PERMISSIONS OWNER_WRITE)

ELSEIF(UNIX)
  install(TARGETS dlEngine 
          LIBRARY DESTINATION bin
          PERMISSIONS OWNER_WRITE)
ENDIF()

install(TARGETS dlSandbox 
        RUNTIME DESTINATION bin
        PERMISSIONS OWNER_WRITE)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/version.txt
        DESTINATION .)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" ver)

string(REGEX MATCH "VERSION_MAJOR ([0-9]*)" _ ${ver})
set(ver_major ${CMAKE_MATCH_1})

string(REGEX MATCH "VERSION_MINOR ([0-9]*)" _ ${ver})
set(ver_minor ${CMAKE_MATCH_1})

string(REGEX MATCH "VERSION_PATCH ([0-9]*)" _ ${ver})
set(ver_patch ${CMAKE_MATCH_1})

message("-- App version: ${ver_major}.${ver_minor}.${ver_patch}")

# Package config
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "Daedalus Development Sandbox")
set(CPACK_PACKAGE_VENDOR "Maksym Yershykov Dev")
set(CPACK_PACKAGE_DESCRIPTION "Development Kit for Daedalus Engine: Daedalus Engine, Sandbox Application and Daedalus Editor")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Development Kit for Daedalus Engine")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${ver_major}")
set(CPACK_PACKAGE_VERSION_MINOR "${ver_minor}")
set(CPACK_PACKAGE_VERSION_PATCH "${ver_patch}")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/install")
include(CPack)