cmake_minimum_required(VERSION 3.16.3)

project(
    dlUnitTests
    VERSION 1.0.0
    LANGUAGES CXX)

set(PROJECT_FILES

    source/main.cpp
    source/DummyTest.cpp
    )

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PROJECT_FILES})

add_executable(${PROJECT_NAME} ${PROJECT_FILES})

# set target properties to hide libs from solution
set_target_properties(
    gtest
    gtest_main
    gmock
    gmock_main

    PROPERTIES FOLDER HiddenTargets)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}../ThirdParty/googletest/googletest/include
)

target_link_libraries(${PROJECT_NAME}
PRIVATE    
    dlEngine
    gtest
)