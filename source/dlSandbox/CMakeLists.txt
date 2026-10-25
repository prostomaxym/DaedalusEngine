cmake_minimum_required(VERSION 3.16.3)

project(
  dlSandbox
  VERSION 1.0.0
  LANGUAGES CXX
  )

set(SCENE_FILES
  scenes/nuke.scene.json
  scenes/anor_londo.scene.json
)

set(PROJECT_FILES
  ${SCENE_FILES}
  ExampleLayer.h
  ExampleLayer.cpp
  ExampleScripts.h
	SceneScripts.h
  SceneScripts.cpp
  SandboxApp.h
	SandboxApp.cpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PROJECT_FILES})

add_executable(${PROJECT_NAME} ${PROJECT_FILES})

foreach(scene_file IN LISTS SCENE_FILES)
	get_filename_component(scene_name "${scene_file}" NAME)
	add_custom_command(OUTPUT "${CMAKE_SOURCE_DIR}/runtime/asset/scenes/${scene_name}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/runtime/asset/scenes"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/${scene_file}"
			"${CMAKE_SOURCE_DIR}/runtime/asset/scenes/${scene_name}"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${scene_file}"
		VERBATIM)
	list(APPEND DEPLOYED_SCENES "${CMAKE_SOURCE_DIR}/runtime/asset/scenes/${scene_name}")
endforeach()
add_custom_target(SandboxScenes DEPENDS ${DEPLOYED_SCENES})
set_target_properties(SandboxScenes PROPERTIES FOLDER Sandbox)
add_dependencies(${PROJECT_NAME} SandboxScenes)

if(WIN32)
    target_sources(dlSandbox PRIVATE daedalus_icon.rc)
endif()

target_link_libraries(${PROJECT_NAME}
PRIVATE
  dlEngine
)