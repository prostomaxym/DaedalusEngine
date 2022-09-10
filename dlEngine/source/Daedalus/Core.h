#pragma once
#include <signal.h>

#ifdef DL_PLATFORM_WINDOWS
	#ifdef DL_BUILD_DLL
		#define DAEDALUS_API __declspec(dllexport)
	#else
		#define DAEDALUS_API __declspec(dllimport)
	#endif
#elif defined DL_PLATFORM_LINUX
	#ifdef DL_BUILD_DLL
		#define DAEDALUS_API __attribute__((visibility("default")))
	#else
		#define DAEDALUS_API
	#endif
#else
	#error Daedalus only support Windows and Linux
#endif


// Evil macros
#ifdef SIGTRAP
#define PLATFORM_DEBUG_BREAK raise(SIGTRAP)
#else
#define PLATFORM_DEBUG_BREAK raise(SIGABRT)
#endif // SIGTRAP

#ifdef DL_ENABLE_ASSERTS
#define DL_ASSERT(x, ...) { if(!(x)) { DL_ERROR("Assertion Failed: {0}", __VA_ARGS__); PLATFORM_DEBUG_BREAK;; } }
#define DL_CORE_ASSERT(x, ...) { if(!(x)) { DL_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); PLATFORM_DEBUG_BREAK;; } }
#else
#define DL_ASSERT(x, ...)
#define DL_CORE_ASSERT(x, ...)
#endif


#define BITMASK(x) (1 << x)
