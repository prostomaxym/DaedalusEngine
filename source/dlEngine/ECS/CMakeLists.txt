cmake_minimum_required(VERSION 3.16.3)

#----------------- Dependencies config --------------------------#
find_package(EnTT CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

#------------------ Project config ------------------------------#
project(
    ECS
    VERSION 1.0.0
    LANGUAGES CXX
    )

set(ECS_PROJECT_FILES
    Components.h
    Entity.h
    Entity.cpp
    Scene.h
    SceneComponents.inl
    Scene.cpp
    NativeScript.h
    NativeScriptComponent.h
    UUID.h
    NativeScripts/CameraController.h
    NativeScripts/CameraController.cpp
    Systems/RenderSystem.h
    Systems/RenderSystem.cpp
)

add_library(${PROJECT_NAME} STATIC ${ECS_PROJECT_FILES})

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
    PRIVATE
        Core
        Renderer
        Utils
        EnTT::EnTT
        glm::glm
    )