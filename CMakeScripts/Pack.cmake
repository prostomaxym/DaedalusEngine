#--------------------- Package config -------------------------#
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" ver)

string(REGEX MATCH "VERSION_MAJOR ([0-9]*)" _ ${ver})
set(ver_major ${CMAKE_MATCH_1})

string(REGEX MATCH "VERSION_MINOR ([0-9]*)" _ ${ver})
set(ver_minor ${CMAKE_MATCH_1})

string(REGEX MATCH "VERSION_PATCH ([0-9]*)" _ ${ver})
set(ver_patch ${CMAKE_MATCH_1})

message("-- Daedalus version: ${ver_major}.${ver_minor}.${ver_patch}")

include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "Daedalus Development Sandbox")
set(CPACK_PACKAGE_VENDOR "Maksym Yershykov Dev")
set(CPACK_PACKAGE_DESCRIPTION "Development Kit for Daedalus Engine: Daedalus Engine, Sandbox Application and Daedalus Editor")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Development Kit for Daedalus Engine")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${ver_major}")
set(CPACK_PACKAGE_VERSION_MINOR "${ver_minor}")
set(CPACK_PACKAGE_VERSION_PATCH "${ver_patch}")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/install")
include(CPack)