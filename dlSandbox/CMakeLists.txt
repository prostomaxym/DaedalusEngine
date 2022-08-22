cmake_minimum_required(VERSION 3.16.3)

project(
  dlSandbox
  VERSION 1.0.0
  LANGUAGES CXX)

set(PROJECT_INCLUDES)

set(PROJECT_SOURCES
	source/SandboxApp.cpp)

set(PROJECT_FILES ${PROJECT_SOURCES} ${PROJECT_INCLUDES})

add_executable(${PROJECT_NAME} ${PROJECT_FILES})

add_compile_definitions(DL_ENABLE_ASSERTS)

target_include_directories(${PROJECT_NAME}
			PUBLIC 
				${CMAKE_CURRENT_SOURCE_DIR}/../dlEngine/source/
                ${CMAKE_CURRENT_SOURCE_DIR}/../dlEngine/ThirdParty/spdlog/include
                ${CMAKE_CURRENT_SOURCE_DIR}/../dlEngine/ThirdParty/glfw/include
                )

target_link_libraries(${PROJECT_NAME}
				PRIVATE
					dlEngine
                    spdlog
                    glfw3
					)		