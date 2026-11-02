cmake_minimum_required(VERSION 3.16.3)
project(dlAndroidProbe LANGUAGES CXX)

if(NOT ANDROID)
	message(FATAL_ERROR "dlAndroidProbe requires the Android NDK toolchain")
endif()

add_executable(dlAndroidProbe AndroidProbe.cpp ShaderProbe.h ShaderProbe.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES AndroidProbe.cpp ShaderProbe.h ShaderProbe.cpp)
target_compile_features(dlAndroidProbe PRIVATE cxx_std_20)
target_link_libraries(dlAndroidProbe PRIVATE EGL GLESv3 log)
