cmake_minimum_required(VERSION 3.16.3)

#----------------- Dependencies config --------------------------#
find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

#------------------ Project config ------------------------------#
project(
    Platform
    VERSION 1.0.0
    LANGUAGES CXX
    )

set(PLATFORM_PROJECT_FILES
    GLFW/GLFWInput.h
    GLFW/GLFWInput.cpp
    GLFW/GLFWWindow.h
    GLFW/GLFWWindow.cpp

    OpenGL/OpenGLContext.h
    OpenGL/OpenGLContext.cpp
    OpenGL/OpenGLFrameBuffer.h
    OpenGL/OpenGLFrameBuffer.cpp
    OpenGL/OpenGLIndexBuffer.h
    OpenGL/OpenGLIndexBuffer.cpp
    OpenGL/OpenGLRendererAPI.h
    OpenGL/OpenGLRendererAPI.cpp
    OpenGL/OpenGLShader.h
    OpenGL/OpenGLShader.cpp
    OpenGL/OpenGLShaderCompiler.h
    OpenGL/OpenGLShaderCompiler.cpp
    OpenGL/OpenGLShaderStorageBuffer.h
    OpenGL/OpenGLShaderStorageBuffer.cpp
    OpenGL/OpenGLTexture.h
    OpenGL/OpenGLTexture.cpp
    OpenGL/OpenGLUniformBuffer.h
    OpenGL/OpenGLUniformBuffer.cpp
    OpenGL/OpenGLVertexArray.h
    OpenGL/OpenGLVertexArray.cpp
    OpenGL/OpenGLVertexBuffer.h
    OpenGL/OpenGLVertexBuffer.cpp

    Windows/WindowsInput.h
    Windows/WindowsWindow.h
    Windows/WindowsWindow.cpp

    Linux/LinuxInput.h
    Linux/LinuxWindow.h
    Linux/LinuxWindow.cpp
)

add_library(${PROJECT_NAME} STATIC ${PLATFORM_PROJECT_FILES})

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
        GLFW_INCLUDE_NONE
    )

target_link_libraries(${PROJECT_NAME}
    PRIVATE    
        ${OPENGL_LIBRARIES}
        glad::glad
        glfw
)
        