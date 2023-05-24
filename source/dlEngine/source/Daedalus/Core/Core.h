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

#define DL_DEBUG_BUILD //TODO: define it from project config
#define DL_DEV_BUILD //TODO: define it from project config

#ifdef SIGTRAP
#define PLATFORM_DEBUG_BREAK raise(SIGTRAP)
#else
#define PLATFORM_DEBUG_BREAK raise(SIGABRT)
#endif // SIGTRAP

#ifdef DL_ENABLE_ASSERTS
#define DL_ASSERT(x, category, ...) { if(!(x)) { Log::Write(Log::Levels::Error, category, "Assertion Failed: {0}", __VA_ARGS__); PLATFORM_DEBUG_BREAK;; } }
#else
#define DL_ASSERT(x, ...)
#endif


#define BITMASK(x) (1 << x)

#define DL_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)