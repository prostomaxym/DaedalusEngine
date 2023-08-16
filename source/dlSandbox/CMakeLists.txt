cmake_minimum_required(VERSION 3.16.3)

project(
  dlSandbox
  VERSION 1.0.0
  LANGUAGES CXX
  )

set(PROJECT_FILES
  source/ExampleLayer.h
  source/ExampleLayer.cpp
  source/ExampleScripts.h
  source/SandboxApp.h
	source/SandboxApp.cpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PROJECT_FILES})

add_executable(${PROJECT_NAME} ${PROJECT_FILES})

if(WIN32)
    target_sources(dlSandbox PRIVATE daedalus_icon.rc)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
PRIVATE
  dlEngine
)