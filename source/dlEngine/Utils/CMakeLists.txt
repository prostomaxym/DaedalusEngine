cmake_minimum_required(VERSION 3.16.3)
find_package(libzip CONFIG REQUIRED)

#------------------ Project config ------------------------------#
project(
    Utils
    VERSION 1.0.0
    LANGUAGES CXX
    )

set(UTILS_PROJECT_FILES
   AssetSourceReader.h
   AssetSourceReader.cpp
   BundledFileInstaller.h
   BundledFileInstaller.cpp
   ContentFingerprint.h
   DeltaTime.h
   FixedStepClock.h
   HighPerformanceRequest.h
   FPSLocker.h
   KeyCodes.h
   Random.h
   Random.cpp
   RingVector.h
   Timer.h
   TouchControls.h
   Utf8.h
   VirtualFileSystem.h
   VirtualFileSystem.cpp
   WorkingDirectory.h
   WorkingDirectory.cpp
)

add_library(${PROJECT_NAME} STATIC ${UTILS_PROJECT_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Engine)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${UTILS_PROJECT_FILES})

target_precompile_headers(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../dlpch.h )

target_compile_definitions(${PROJECT_NAME} 
    PRIVATE 
        DL_BUILD_STATIC
    )

target_include_directories(${PROJECT_NAME}
    PUBLIC 
	    ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Debug
        libzip::zip
    )