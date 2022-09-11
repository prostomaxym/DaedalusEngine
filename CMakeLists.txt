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
        
# Packing config
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/install")
include(CPack)