cmake_minimum_required(VERSION 3.16.3)

#------------------ Project config ------------------------------#
project(
    Core
    VERSION 1.0.0
    LANGUAGES CXX
    )

set(CORE_PROJECT_FILES
    Application.h
    Application.cpp
    EntryPoint.h
    Input.h
    Input.cpp
    Layer.h
    Layer.cpp
    LayerStack.h
    LayerStack.cpp
    Window.h  
)

add_library(${PROJECT_NAME} STATIC ${CORE_PROJECT_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Engine)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${CORE_PROJECT_FILES})

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
        Renderer
        Events
        Platform
        Utils
    )