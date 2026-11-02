cmake_minimum_required(VERSION 3.16.3)

#----------------- Dependencies config --------------------------#
find_package(imgui CONFIG REQUIRED)
if(NOT ANDROID)
    find_package(OpenGL REQUIRED)
    find_package(glad CONFIG REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
endif()

#------------------ Project config ------------------------------#
project(
    Platform
    VERSION 1.0.0
    LANGUAGES CXX
    )

set(PLATFORM_PROJECT_FILES
    Platform.h
    Platform.cpp
    ResourcesMonitor.h

    ImGuiBackend.h
    OpenGL/OpenGLHeaders.h
    OpenGL/OpenGLTextureBinding.h
    OpenGL/OpenGLFrameBuffer.h
    OpenGL/OpenGLFrameBuffer.cpp
    OpenGL/OpenGLGPUTimer.h
    OpenGL/OpenGLGPUTimer.cpp
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
    Windows/WindowsResourcesMonitor.h
    Windows/WindowsResourcesMonitor.cpp

    Linux/LinuxInput.h
    Linux/LinuxWindow.h
    Linux/LinuxWindow.cpp
    Linux/LinuxResourcesMonitor.h
    Linux/LinuxResourcesMonitor.cpp

)

if(ANDROID)
    list(APPEND PLATFORM_PROJECT_FILES
        Android/AndroidBootstrap.h Android/AndroidBootstrap.cpp
        Android/AndroidRuntime.h Android/AndroidRuntime.cpp
        Android/AndroidWindow.h Android/AndroidWindow.cpp
        Android/AndroidInput.h Android/AndroidInput.cpp
        Android/AndroidContext.h Android/AndroidContext.cpp
        Android/AndroidResourcesMonitor.cpp
        Android/AndroidLogSink.h
        Android/AndroidImGui.cpp)
else()
    list(APPEND PLATFORM_PROJECT_FILES
        ResourcesMonitor.cpp
        GLFW/GamepadButtonEvents.h
        GLFW/GLFWInput.h GLFW/GLFWInput.cpp
        GLFW/GLFWWindow.h GLFW/GLFWWindow.cpp GLFW/GLFWImGui.cpp
        OpenGL/OpenGLContext.h OpenGL/OpenGLContext.cpp
        Nvidia/NvidiaGPUMonitor.h Nvidia/NvidiaGPUMonitor.cpp)
endif()

add_library(${PROJECT_NAME} STATIC ${PLATFORM_PROJECT_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Engine)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PLATFORM_PROJECT_FILES})

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
        Config
        imgui::imgui
)

if(ANDROID)
    target_link_libraries(${PROJECT_NAME} PRIVATE AndroidNativeGlueHeaders EGL GLESv3 android log)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL_LIBRARIES} glad::glad glfw)
endif()

IF(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE pdh)
ENDIF()