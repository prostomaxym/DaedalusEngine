cmake_minimum_required(VERSION 3.16.3)

#----------------- Dependencies config --------------------------#
find_package(EnTT CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-lua CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)

#------------------ Project config ------------------------------#
project(
    ECS
    VERSION 1.0.0
    LANGUAGES CXX
    )

set(ECS_PROJECT_FILES
    Components.h
    PhysicsBodyComponent.h
    PhysicsGameplayComponents.h
    Entity.h
    Entity.cpp
    Scene.h
    SceneComponents.inl
    SceneComponents.cpp
    Scene.cpp
    SceneHierarchy.cpp
    SceneManager.h
    SceneManager.cpp
    Serialization/SceneRecord.h
    Serialization/SceneJson.h
    Serialization/SceneJson.cpp
    Serialization/SceneMigration.cpp
    Serialization/SceneComposer.h
    Serialization/SceneComposer.cpp
    Serialization/ScriptRegistry.h
    Serialization/ScriptRegistry.cpp
    Serialization/SceneSerializer.h
    Serialization/SceneSerializer.cpp
    NativeScript.h
    NativeScriptComponent.h
    Scripting/LuaRuntime.h
    Scripting/LuaRuntime.cpp
    Scripting/LuaScript.h
    Scripting/LuaScript.cpp
    UUID.h
    NativeScripts/CameraController.h
    NativeScripts/CameraController.cpp
    Systems/RenderSystem.h
    Systems/RenderSystem.cpp
    Systems/TransformPreparation.h
    Systems/TransformPreparation.cpp
    Systems/PhysicsSystem.h
    Systems/PhysicsSystem.cpp
    Systems/PhysicsGameplaySystem.cpp
)

add_library(${PROJECT_NAME} STATIC ${ECS_PROJECT_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Engine)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ECS_PROJECT_FILES})

target_precompile_headers(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../dlpch.h )

target_include_directories(${PROJECT_NAME}
    PUBLIC 
	    ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

target_compile_definitions(${PROJECT_NAME} 
    PRIVATE 
        DL_BUILD_STATIC
        SOL_USING_CXX_LUA=1
        SOL_ALL_SAFETIES_ON=1
        SOL_EXCEPTIONS_SAFE_PROPAGATION=1
    )

target_link_libraries(${PROJECT_NAME}  
    PRIVATE
        Core
        Events
        Renderer
        Threads
        Utils
        EnTT::EnTT
        glm::glm
        nlohmann_json::nlohmann_json
        unofficial::lua::lua-cpp
        sol2::sol2
    )