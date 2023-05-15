cmake_minimum_required(VERSION 3.16.3)

#----------------- Dependencies config --------------------------#
#PATHS "${CMAKE_SOURCE_DIR}/vcpkg_installed"
find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb_c_lexer.h")
find_package(assimp CONFIG REQUIRED)


#------------------ Project config ------------------------------#
project(
    dlEngine
    VERSION 1.0.0
    LANGUAGES CXX
    )

# Import project files
include(${CMAKE_CURRENT_SOURCE_DIR}/ProjectFiles.cmake)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PROJECT_FILES})

add_library(${PROJECT_NAME} SHARED ${PROJECT_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} 
    PUBLIC
        GLFW_INCLUDE_NONE
        $<$<CONFIG:Debug>:DL_ENABLE_ASSERTS>
        $<$<CONFIG:RelWithDebInfo>:DL_ENABLE_ASSERTS>
    PRIVATE 
        DL_BUILD_DLL
        )

target_precompile_headers(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/source/dlpch.h )

target_include_directories(${PROJECT_NAME}
    PUBLIC 
	    ${CMAKE_CURRENT_SOURCE_DIR}/source/
    PRIVATE 
        ${STB_INCLUDE_DIRS}
    )

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        spdlog::spdlog
        imgui::imgui
        imguizmo::imguizmo
    PRIVATE    
        ${OPENGL_LIBRARIES}
        glad::glad
        glfw
        glm::glm
        assimp::assimp
    )