cmake_minimum_required(VERSION 3.16.3)

#------------------ Project config ------------------------------#
project(
    dlShaders
    VERSION 1.0.0
    LANGUAGES CXX
    )

set(PROJECT_FILES
    LightSSBO.hglsl
    Scene.hglsl

    Blur.glsl
    Cubemap.glsl
    DeferredGeometryPass.glsl
    DeferredLightPass.glsl
    ForwardPass.glsl
    ShadowPass.glsl
    SSAO.glsl
    TextureArrayToGrayScale.glsl
    TextureToGrayScale.glsl
    DepthToGrayScale.glsl
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PROJECT_FILES})

add_custom_target(${PROJECT_NAME} SOURCES ${PROJECT_FILES})

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/runtime/shader
            
    COMMAND ${CMAKE_COMMAND} -E rm
            ${CMAKE_SOURCE_DIR}/runtime/shader/CMakeLists.txt

    COMMENT "Copying shaders to runtime folder"
)