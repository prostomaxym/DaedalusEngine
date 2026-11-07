include_guard(GLOBAL)

option(ENABLE_SANITIZERS "Instrument project targets with Linux ASan/UBSan" OFF)

if(ENABLE_SANITIZERS)
	if(ANDROID OR NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
		message(FATAL_ERROR "ENABLE_SANITIZERS requires a desktop Linux build")
	endif()
	if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
		message(FATAL_ERROR "ENABLE_SANITIZERS requires GCC or Clang")
	endif()
endif()

function(daedalus_enable_sanitizers target)
	if(NOT ENABLE_SANITIZERS)
		return()
	endif()

	get_target_property(is_imported ${target} IMPORTED)
	if(is_imported)
		message(FATAL_ERROR "Sanitizers must not be applied to imported target ${target}")
	endif()

	target_compile_options(${target} PRIVATE
		-fsanitize=address,undefined
		-fno-sanitize-recover=all
		-fno-omit-frame-pointer
		-g
		-O1
	)
	target_link_options(${target} PRIVATE -fsanitize=address,undefined)
endfunction()
