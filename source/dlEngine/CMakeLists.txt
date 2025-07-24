cmake_minimum_required(VERSION 3.16.3)

#----------------- Dependencies config --------------------------#
#PATHS "${CMAKE_SOURCE_DIR}/vcpkg_installed"

find_package(fmt CONFIG REQUIRED)

#------------------ Project config ------------------------------#
project(
    dlEngine
    VERSION 1.0.0
    LANGUAGES CXX
    )

add_subdirectory(Config)
add_subdirectory(Core)
add_subdirectory(Debug)
add_subdirectory(ECS)
add_subdirectory(Events)
add_subdirectory(Platform)
add_subdirectory(Renderer)
add_subdirectory(Threads)
add_subdirectory(Utils)

set(PROJECT_FILES
    Daedalus.h
    Daedalus.cpp
    dlpch.h
    dlpch.cpp
    DummyTest.h
    Macros.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PROJECT_FILES})

add_library(${PROJECT_NAME} SHARED ${PROJECT_FILES})

target_compile_definitions(${PROJECT_NAME} 
    PUBLIC
        $<$<CONFIG:Debug>:DL_ENABLE_ASSERTS>
        NOMINMAX
    PRIVATE 
        DL_BUILD_DLL     
        )

target_precompile_headers(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dlpch.h )

target_include_directories(${PROJECT_NAME}
    PUBLIC 
	    ${CMAKE_CURRENT_SOURCE_DIR}
    )

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        fmt::fmt
    PRIVATE
        Config
        Core
        Debug
        ECS
        Events
        Platform
        Renderer
        Threads
        Utils
    )

add_custom_command(
    TARGET dlEngine
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/../shader
            ${CMAKE_SOURCE_DIR}/runtime/shader
)