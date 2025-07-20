#pragma once

//Force higher performance GPU usage
#ifdef DL_PLATFORM_WINDOWS

#include <Windows.h>

extern "C"
{
	_declspec(dllexport) DWORD NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif