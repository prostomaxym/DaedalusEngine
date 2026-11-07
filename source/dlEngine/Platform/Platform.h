#pragma once

#include <Macros.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>

namespace Daedalus {
	class Window;
	class Input;
	struct WindowProps;

	class DAEDALUS_API Platform
	{
	public:
		static std::unique_ptr<Window> createWindow(const WindowProps& props);
		static std::unique_ptr<Input> CreateInputSystem();
		static void SetupSegfaultHandler();
		[[nodiscard]] static bool GetExecutablePath(std::filesystem::path& path, std::string& error);

		static std::pair<int, int> GetMonitorResolution();

		static void PreciseThreadSleep(long long nanoseconds);

		static std::string FormatTimestamp(const std::chrono::system_clock::time_point& tp);
	};
}
