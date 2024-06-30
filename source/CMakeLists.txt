cmake_minimum_required(VERSION 3.16.3)

# Subprojects config
add_subdirectory(dlEngine)
add_subdirectory(dlSandbox)
add_subdirectory(shader)

# Tests setup
if (ENABLE_TESTS)
    enable_testing()
    add_subdirectory(dlUnitTests)
endif()
