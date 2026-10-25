cmake_minimum_required(VERSION 3.16.3)

#----------------- Dependencies config --------------------------#
find_package(GTest CONFIG REQUIRED)


#----------------- Project config --------------------------#
project(
    dlUnitTests
    VERSION 1.0.0
    LANGUAGES CXX
    )

set(CONFIG_TEST_FILES
    ConfigTests/KeybindConfigTest.cpp
    )

set(CORE_TEST_FILES
    CoreTests/LayerStackTest.cpp
    )

set(DEBUG_TEST_FILES
    DebugTests/LogTest.cpp
    )

set(ECS_TEST_FILES
    ECSTests/EntityTest.cpp
    ECSTests/NativeScriptTest.cpp
    ECSTests/SceneTest.cpp
    ECSTests/TransformComponentTest.cpp
    ECSTests/UUIDTest.cpp
    )

set(EVENTS_TEST_FILES
    EventsTests/EventDispatcherTest.cpp
    EventsTests/EventTest.cpp
    EventsTests/GamepadEventTest.cpp
    )

set(PHYSICS_TEST_FILES
    PhysicsTests/PhysicsTest.cpp
    )

set(RENDERER_TEST_FILES
    RendererTests/BufferLayoutTest.cpp
    RendererTests/CameraTest.cpp
    RendererTests/DirectionalLightSourceTest.cpp
    RendererTests/GeometryTest.cpp
    RendererTests/LightSourceTest.cpp
    )

set(THREADS_TEST_FILES
    ThreadsTests/ThreadPoolTest.cpp
    ThreadsTests/ThreadJoinerTest.cpp
    ThreadsTests/ThreadRunnerTest.cpp
    )

set(UTILS_TEST_FILES
    UtilsTests/DeltaTimeTest.cpp
    UtilsTests/FPSLockerTest.cpp
    UtilsTests/KeyCodesTest.cpp
    UtilsTests/RandomTest.cpp
    UtilsTests/RingVectorTest.cpp
    UtilsTests/TimerTest.cpp
    )

set(PROJECT_FILES

    main.cpp
    ${CONFIG_TEST_FILES}
    ${CORE_TEST_FILES}
    ${DEBUG_TEST_FILES}
    ${ECS_TEST_FILES}
    ${EVENTS_TEST_FILES}
    ${PHYSICS_TEST_FILES}
    ${RENDERER_TEST_FILES}
    ${THREADS_TEST_FILES}
    ${UTILS_TEST_FILES}
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
