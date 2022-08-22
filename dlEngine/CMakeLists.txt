cmake_minimum_required(VERSION 3.16.3)

add_subdirectory(ThirdParty/glfw3)
add_subdirectory(ThirdParty/spdlog)

project(
  dlEngine
  VERSION 1.0.0
  LANGUAGES CXX)

set(PROJECT_INCLUDES

    source/Daedalus.h
    source/dlpch.h
    source/DummyTest.h

    source/Daedalus/Application.h
    source/Daedalus/Core.h
    source/Daedalus/EntryPoint.h
    source/Daedalus/Iwindow.h
    source/Daedalus/Log.h
    source/Daedalus/Events/ApplicationEvent.h
    source/Daedalus/Events/Event.h
    source/Daedalus/Events/EventDispatcher.h
    source/Daedalus/Events/EventTable.h
    source/Daedalus/Events/KeyEvent.h
    source/Daedalus/Events/MouseEvent.h
    
    source/Platform/Windows/WindowsWindow.h
    )

set(PROJECT_SOURCES

    source/dlpch.cpp

	source/Daedalus/Application.cpp
    source/Daedalus/Log.cpp
    
    source/Platform/Windows/WindowsWindow.cpp
    )

set(PROJECT_FILES ${PROJECT_SOURCES} ${PROJECT_INCLUDES})

add_library(${PROJECT_NAME} SHARED ${PROJECT_FILES})

target_precompile_headers(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source/dlpch.h )

add_compile_definitions(DL_ENABLE_ASSERTS
                        DL_BUILD_DLL)

target_include_directories(${PROJECT_NAME}
			PRIVATE 
				${CMAKE_CURRENT_SOURCE_DIR}/source/
                ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/spdlog/include
                ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glfw3/include
                )

target_link_libraries(${PROJECT_NAME}
				PRIVATE    
                    spdlog
                    opengl32
                    glfw3
					)		