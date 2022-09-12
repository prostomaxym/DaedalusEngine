cmake_minimum_required(VERSION 3.16.3)

project(
  dlSandbox
  VERSION 1.0.0
  LANGUAGES CXX
  )

set(PROJECT_FILES
	source/SandboxApp.cpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PROJECT_FILES})

add_executable(${PROJECT_NAME} ${PROJECT_FILES})

add_compile_definitions(DL_ENABLE_ASSERTS)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
PRIVATE
  dlEngine
  spdlog
)