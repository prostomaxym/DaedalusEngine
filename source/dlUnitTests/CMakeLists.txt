cmake_minimum_required(VERSION 3.16.3)

#----------------- Dependencies config --------------------------#
find_package(GTest CONFIG REQUIRED)


#----------------- Project config --------------------------#
project(
    dlUnitTests
    VERSION 1.0.0
    LANGUAGES CXX
    )

set(UTILS_TEST_FILES
    UtilsTests/DeltaTimeTest.cpp
    UtilsTests/RandomTest.cpp
    UtilsTests/RingVectorTest.cpp
    UtilsTests/TimerTest.cpp
    )

set(EVENTS_TEST_FILES
    EventsTests/EventDispatcherTest.cpp
    EventsTests/EventTest.cpp
    )

set(PROJECT_FILES

    main.cpp
    DummyTest.cpp
    ${UTILS_TEST_FILES}
    ${EVENTS_TEST_FILES}
    )

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PROJECT_FILES})

add_executable(${PROJECT_NAME} ${PROJECT_FILES})

target_link_libraries(${PROJECT_NAME}
PRIVATE    
    dlEngine
    GTest::gmock
    GTest::gmock_main
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME "${PROJECT_NAME}" COMMAND "${PROJECT_NAME}")