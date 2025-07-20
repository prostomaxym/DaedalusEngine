cmake_minimum_required(VERSION 3.16.3)

#------------------ Project config ------------------------------#
project(
    Utils
    VERSION 1.0.0
    LANGUAGES CXX
    )

set(UTILS_PROJECT_FILES
   DeltaTime.h
   HighPerformanceRequest.h
   FPSLocker.h
   KeyCodes.h
   Random.h
   Random.cpp
   Timer.h
   WorkingDirectory.h
   WorkingDirectory.cpp
)

add_library(${PROJECT_NAME} STATIC ${UTILS_PROJECT_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER dlEngine)

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
    )