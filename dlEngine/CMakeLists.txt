cmake_minimum_required(VERSION 3.16.3)

IF(UNIX)
    execute_process(COMMAND sudo apt update
                    COMMAND sudo apt install libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev)
ENDIF()

set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_EXAMPLES OFF)

add_subdirectory(ThirdParty/glfw3)
add_subdirectory(ThirdParty/spdlog)

project(
    dlEngine
    VERSION 1.0.0
    LANGUAGES CXX)

set(PROJECT_FILES

    source/Daedalus.h
    source/dlpch.h
    source/dlpch.cpp
    source/DummyTest.h

    source/Daedalus/Application.h
    source/Daedalus/Application.cpp
    source/Daedalus/Core.h
    source/Daedalus/EntryPoint.h
    source/Daedalus/IWindow.h
    source/Daedalus/Log.h
    source/Daedalus/Log.cpp
    source/Daedalus/Events/ApplicationEvent.h
    source/Daedalus/Events/Event.h
    source/Daedalus/Events/EventDispatcher.h
    source/Daedalus/Events/EventTable.h
    source/Daedalus/Events/KeyEvent.h
    source/Daedalus/Events/MouseEvent.h
    
    source/Platform/GLFWWindow.h
    source/Platform/GLFWWindow.cpp

    source/Platform/Windows/WindowsWindow.h
    source/Platform/Windows/WindowsWindow.cpp

    source/Platform/Linux/LinuxWindow.h
    source/Platform/Linux/LinuxWindow.cpp
    )

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PROJECT_FILES})

add_library(${PROJECT_NAME} SHARED ${PROJECT_FILES})

target_precompile_headers(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source/dlpch.h )

add_compile_definitions(
    DL_ENABLE_ASSERTS
    DL_BUILD_DLL)

# set target properties to hide libs from solution
set_target_properties(
    spdlog 
    glfw 
    uninstall 
    update_mappings 

    PROPERTIES FOLDER HiddenTargets)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/source/
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/spdlog/include
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glfw3/include
)

target_link_libraries(${PROJECT_NAME}
PUBLIC
    spdlog
PRIVATE    
    opengl32
    glfw
)