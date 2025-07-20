cmake_minimum_required(VERSION 3.16.3)

#----------------- Dependencies config --------------------------#
find_package(spdlog CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

#------------------ Project config ------------------------------#
project(
    Debug
    VERSION 1.0.0
    LANGUAGES CXX
    )

set(DEBUG_PROJECT_FILES
    Log.h
    Log.cpp
    ImGuiLayer.h
    ImGuiLayer.cpp
)

add_library(${PROJECT_NAME} STATIC ${DEBUG_PROJECT_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER dlEngine)

target_precompile_headers(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../dlpch.h )

target_include_directories(${PROJECT_NAME}
    PUBLIC 
	    ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

target_compile_definitions(${PROJECT_NAME} 
    PRIVATE 
        DL_BUILD_STATIC
    )
    
target_link_libraries(${PROJECT_NAME} 
    PUBLIC
        fmt::fmt
    PRIVATE
        spdlog::spdlog
        imgui::imgui
        imguizmo::imguizmo     
    )
