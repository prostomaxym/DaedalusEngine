#pragma once

#ifdef DL_PLATFORM_WINDOWS
	#ifdef DL_BUILD_DLL
		#define DAEDALUS_API __declspec(dllexport)
	#else
		#define DAEDALUS_API __declspec(dllimport)
	#endif
#else
	#error Daedalus only support Windows
#endif


// Evil macros
#define BIT(x) (1 << x)
