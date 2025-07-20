cmake_minimum_required(VERSION 3.16.3)

#------------------ Project config ------------------------------#
project(
    Events
    VERSION 1.0.0
    LANGUAGES CXX
    )

set(EVENTS_PROJECT_FILES
    ApplicationEvent.h
    Event.h
    EventDispatcher.h
    EventTable.h
    KeyEvent.h
    MouseEvent.h
    GamepadEvent.h
    EventsEngine.h
    EventsEngine.cpp
)

add_library(${PROJECT_NAME} STATIC ${EVENTS_PROJECT_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Engine)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${EVENTS_PROJECT_FILES})

target_precompile_headers(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../dlpch.h )

target_compile_definitions(${PROJECT_NAME} 
    PRIVATE 
        DL_BUILD_STATIC
    )
  
target_include_directories(${PROJECT_NAME}
    PUBLIC 
	    ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Debug
    )