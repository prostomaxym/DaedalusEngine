cmake_minimum_required(VERSION 3.16.3)

#------------------ Project config ------------------------------#
project(
    Threads
    VERSION 1.0.0
    LANGUAGES CXX
    )

set(THREADS_PROJECT_FILES
    DaedalusThreads.h
    ThreadJoiner.h
    ThreadJoiner.cpp
    ThreadRunner.h
    ThreadRunner.cpp
    ThreadPool.h
    ThreadPool.cpp
)

add_library(${PROJECT_NAME} STATIC ${THREADS_PROJECT_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Engine)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${THREADS_PROJECT_FILES})

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