cmake_minimum_required(VERSION 3.16.3)

#----------------- Dependencies config --------------------------#
find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)


#------------------ Project config ------------------------------#
project(
    dlEngine
    VERSION 1.0.0
    LANGUAGES CXX
    )

add_compile_definitions(
    DL_BUILD_DLL
    GLFW_INCLUDE_NONE
    )

add_compile_definitions(
    $<$<CONFIG:Debug>:DL_ENABLE_ASSERTS>
    $<$<CONFIG:RelWithDebInfo>:DL_ENABLE_ASSERTS>
    )

set(PROJECT_FILES

    source/Daedalus.h
    source/dlpch.h
    source/dlpch.cpp
    source/DummyTest.h

    source/Daedalus/Core/Application.h
    source/Daedalus/Core/Application.cpp
    source/Daedalus/Core/Core.h
    source/Daedalus/Core/EntryPoint.h
    source/Daedalus/Core/IWindow.h
    source/Daedalus/Core/Layer.h
    source/Daedalus/Core/Layer.cpp
    source/Daedalus/Core/LayerStack.h
    source/Daedalus/Core/LayerStack.cpp
    source/Daedalus/Core/Log.h
    source/Daedalus/Core/Log.cpp
    
    source/Daedalus/Events/ApplicationEvent.h
    source/Daedalus/Events/Event.h
    source/Daedalus/Events/EventDispatcher.h
    source/Daedalus/Events/EventTable.h
    source/Daedalus/Events/KeyEvent.h
    source/Daedalus/Events/MouseEvent.h
    source/Daedalus/ImGui/ImGuiLayer.h
    source/Daedalus/ImGui/ImGuiLayer.cpp
    
    source/Platform/GLFWWindow.h
    source/Platform/GLFWWindow.cpp

    source/Platform/Windows/WindowsWindow.h
    source/Platform/Windows/WindowsWindow.cpp

    source/Platform/Linux/LinuxWindow.h
    source/Platform/Linux/LinuxWindow.cpp
    )

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PROJECT_FILES})

add_library(${PROJECT_NAME} SHARED ${PROJECT_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_NAME})

target_precompile_headers(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source/dlpch.h )

target_include_directories(${PROJECT_NAME}
PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/source/
)

target_link_libraries(${PROJECT_NAME}
PUBLIC
    spdlog::spdlog
    imgui::imgui
PRIVATE    
    ${OPENGL_LIBRARIES}
    glad::glad
    glfw
)