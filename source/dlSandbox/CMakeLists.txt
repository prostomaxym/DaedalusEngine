cmake_minimum_required(VERSION 3.16.3)

project(
  dlSandbox
  VERSION 1.0.0
  LANGUAGES CXX
  )

set(PROJECT_FILES
  ExampleLayer.h
  ExampleLayer.cpp
  ExampleScripts.h
	SceneScripts.h
  SceneScripts.cpp
  SandboxApp.h
	SandboxApp.cpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PROJECT_FILES})

if(ANDROID)
	add_library(AndroidNativeGlue STATIC "${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c")
	target_link_libraries(AndroidNativeGlue PUBLIC AndroidNativeGlueHeaders android log)
	set_target_properties(AndroidNativeGlue PROPERTIES POSITION_INDEPENDENT_CODE ON)
	add_library(${PROJECT_NAME} SHARED ${PROJECT_FILES} AndroidEntry.cpp)
	target_link_libraries(${PROJECT_NAME} PRIVATE AndroidNativeGlue)
	target_link_options(${PROJECT_NAME} PRIVATE "-Wl,-u,ANativeActivity_onCreate")
else()
	add_executable(${PROJECT_NAME} ${PROJECT_FILES})
endif()

if(WIN32)
    target_sources(dlSandbox PRIVATE daedalus_icon.rc)
endif()

target_link_libraries(${PROJECT_NAME}
PRIVATE
  dlEngine
)