cmake_minimum_required(VERSION 3.16.3)

#----------------- Dependencies config --------------------------#
  find_package(Bullet CONFIG REQUIRED)
  find_package(glm CONFIG REQUIRED)

#------------------ Project config ------------------------------#
project(
    Physics
    VERSION 1.0.0
    LANGUAGES CXX
    )

set(PHYSICS_PROJECT_FILES
    Physics.h
    Physics.cpp
    PhysicsWorld.h
    PhysicsWorld.cpp
)

add_library(${PROJECT_NAME} STATIC ${PHYSICS_PROJECT_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Engine)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PHYSICS_PROJECT_FILES})

target_precompile_headers(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../dlpch.h )

target_include_directories(${PROJECT_NAME}
    PUBLIC 
	    ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

target_compile_definitions(${PROJECT_NAME} 
    PRIVATE 
        DL_BUILD_STATIC
    )

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        glm::glm
    PRIVATE     
        ${BULLET_LIBRARIES}  
    )