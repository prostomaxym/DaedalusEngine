#pragma once

#include <Macros.h>
#include <Core/Window.h>
#include <Core/Input.h>

#include <memory>

namespace Daedalus {

	class DAEDALUS_API Platform
	{
	public:
		static std::unique_ptr<Window> createWindow(const WindowProps& props);
		static void InitInputSystem();

		static std::pair<int, int> GetMonitorResolution();

		static void PreciseThreadSleep(long long nanoseconds);

		static inline std::string FormatTimestamp(const std::chrono::system_clock::time_point& tp);
	};
}
