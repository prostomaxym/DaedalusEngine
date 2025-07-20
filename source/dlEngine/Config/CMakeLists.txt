cmake_minimum_required(VERSION 3.16.3)

#----------------- Dependencies config --------------------------#
find_path(SIMPLEINI_INCLUDE_DIRS "SimpleIni.h")

#------------------ Project config ------------------------------#
project(
    Config
    VERSION 1.0.0
    LANGUAGES CXX
    )

set(CONFIG_PROJECT_FILES
    GraphicsConfig.h
    GraphicsConfig.cpp
    KeybindConfig.h
    KeybindConfig.cpp
    PathConfig.cpp
    PathConfig.h
    PathConfig.cpp
)


add_library(${PROJECT_NAME} STATIC ${CONFIG_PROJECT_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER dlEngine)

target_precompile_headers(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../dlpch.h )

target_compile_definitions(${PROJECT_NAME} 
    PRIVATE 
        DL_BUILD_STATIC
    )

target_include_directories(${PROJECT_NAME}
    PUBLIC 
	    ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE 
        ${SIMPLEINI_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    )