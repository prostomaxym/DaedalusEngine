file(READ "${CMAKE_SOURCE_DIR}/version.txt" version_text)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/version.txt")

foreach(version_part MAJOR MINOR PATCH)
	string(REGEX MATCH "VERSION_${version_part} ([0-9]+)" version_match "${version_text}")
	if(NOT version_match)
		message(FATAL_ERROR "Missing or invalid VERSION_${version_part} in version.txt")
	endif()
	set(DAEDALUS_VERSION_${version_part} "${CMAKE_MATCH_1}")
endforeach()

set(DAEDALUS_VERSION "${DAEDALUS_VERSION_MAJOR}.${DAEDALUS_VERSION_MINOR}.${DAEDALUS_VERSION_PATCH}")
