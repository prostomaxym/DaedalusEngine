#include "Platform.h"

#include "Linux/LinuxInput.h"
#include "Linux/LinuxWindow.h"
#include "Windows/WindowsInput.h"
#include "Windows/WindowsWindow.h"

#ifdef DL_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <thread>

namespace
{
	struct HandleDeleter
	{
		void operator()(HANDLE handle) const
		{
			if (handle && handle != INVALID_HANDLE_VALUE)
			{
				CloseHandle(handle);
			}
		}
	};
	using UniqueHandle = std::unique_ptr<std::remove_pointer<HANDLE>::type, HandleDeleter>;
	UniqueHandle timer(CreateWaitableTimerEx(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, EVENT_ALL_ACCESS));
}
#endif

using namespace Daedalus;

Input* Input::s_instance = nullptr;

std::unique_ptr<Window> Platform::createWindow(const WindowProps& props)
{
#ifdef DL_PLATFORM_WINDOWS
	return std::make_unique<WindowsWindow>(props);
#elif defined DL_PLATFORM_LINUX
	return std::make_unique<LinuxWindow>(props);
#else
	static_assert(false, "Unsupported Platfrom")
#endif
}

void Platform::InitInputSystem()
{
#ifdef DL_PLATFORM_WINDOWS
	Input::CreateInstance(new WindowsInput());
#elif defined DL_PLATFORM_LINUX
	Input::CreateInstance(new LinuxInput());
#else
	static_assert(false, "Unsupported Platfrom")
#endif
}

std::pair<int, int> Platform::GetMonitorResolution()
{
#ifdef DL_PLATFORM_WINDOWS
	return WindowsWindow::GetMonitorResolution();
#elif defined DL_PLATFORM_LINUX
	return LinuxWindow::GetMonitorResolution();
#else
	static_assert(false, "Unsupported Platfrom")
#endif
	
}

void Platform::PreciseThreadSleep(long long ns)
{
#ifdef DL_PLATFORM_WINDOWS
	const auto timer_ptr = timer.get();
    LARGE_INTEGER li;

    if (timer == nullptr)
        return;

    li.QuadPart = -ns / 100ll;
    if (!SetWaitableTimer(timer_ptr, &li, 0, NULL, NULL, FALSE))
    {
        return;
    }
    WaitForSingleObject(timer_ptr, INFINITE);
#elif defined DL_PLATFORM_LINUX //TODO: not implemented, using nonprecise timer
	std::this_thread::sleep_for(ns);
#else
	static_assert(false, "Unsupported Platfrom")
#endif
}

std::string Platform::FormatTimestamp(const std::chrono::system_clock::time_point& tp)
{
	const std::time_t time = std::chrono::system_clock::to_time_t(tp);
	std::tm local_tm;
#if defined(DL_PLATFORM_WINDOWS)
	localtime_s(&local_tm, &time);
#else
	localtime_r(&time, &local_tm);
#endif
	std::ostringstream oss;
	oss << std::put_time(&local_tm, "%H:%M:%S");
	return oss.str();
}
