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

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_NAME})

find_package(GTest CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}
PRIVATE    
    dlEngine
    GTest::gmock
    GTest::gmock_main
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME "${PROJECT_NAME}" COMMAND "${PROJECT_NAME}")