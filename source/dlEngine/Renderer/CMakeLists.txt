cmake_minimum_required(VERSION 3.16.3)

#----------------- Dependencies config --------------------------#
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

#------------------ Project config ------------------------------#
project(
    Renderer
    VERSION 1.0.0
    LANGUAGES CXX
    )

set(RENDERER_PROJECT_FILES
    API/RenderConstants.h
    API/RenderCommand.h
    API/RenderCommand.cpp
    API/Renderer.h
    API/Renderer.cpp
    API/RendererAPI.h
    API/RendererAPI.cpp

    Buffers/SSAO.h
    Buffers/SSAO.cpp

    Objects/Camera.h
    Objects/Camera.cpp
    Objects/Geometry.h
    Objects/Geometry.cpp
    Objects/Cubemap.h
    Objects/Cubemap.cpp
    Objects/OrthographicCamera.h
    Objects/OrthographicCamera.cpp
    Objects/LightSource.h
    Objects/LightSource.cpp
    Objects/PerspectiveCamera.h
    Objects/PerspectiveCamera.cpp
    Objects/DirectionalLightSource.h
    Objects/DirectionalLightSource.cpp
    Objects/SpotLightSource.h
    Objects/SpotLightSource.cpp
    Objects/PointLightSource.h
    Objects/PointLightSource.cpp

    Passes/DeferredGeometryPass.h
    Passes/DeferredGeometryPass.cpp
    Passes/DeferredLightPass.h
    Passes/DeferredLightPass.cpp
    Passes/SSAOPass.h
    Passes/SSAOPass.cpp
    Passes/ShadowPass.h
    Passes/ShadowPass.cpp
    Passes/SkyboxPass.h
    Passes/SkyboxPass.cpp

    Primitives/BufferLayout.h
    Primitives/GraphicsContext.h
    Primitives/GraphicsContext.cpp
    Primitives/FrameBuffer.h
    Primitives/FrameBuffer.cpp
    Primitives/IndexBuffer.h
    Primitives/IndexBuffer.cpp
    Primitives/ShaderStorageBuffer.h
    Primitives/ShaderStorageBuffer.cpp
    Primitives/UniformBuffer.h
    Primitives/UniformBuffer.cpp
    Primitives/VertexArray.h
    Primitives/VertexArray.cpp
    Primitives/VertexBuffer.h
    Primitives/VertexBuffer.cpp

    Resourses/Material.h
    Resourses/Material.cpp
    Resourses/Mesh.h
    Resourses/Mesh.cpp
    Resourses/Model.h
    Resourses/Model.cpp
    Resourses/Shader.h
    Resourses/Shader.cpp
    Resourses/ShaderLibrary.h
    Resourses/ShaderLibrary.cpp
    Resourses/Texture.h
    Resourses/Texture.cpp
)

add_library(${PROJECT_NAME} STATIC ${RENDERER_PROJECT_FILES})

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
        glm::glm
        assimp::assimp
    )