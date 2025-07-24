#pragma once

#ifdef DL_PLATFORM_LINUX

#include "Platform/GLFW/GLFWWindow.h"

namespace Daedalus {

	class LinuxWindow final : public GLFWWindow
	{
	public:
		LinuxWindow(const WindowProps& props);
		~LinuxWindow() = default;

		static std::pair<int, int> GetMonitorResolution() { return GLFWWindow::GetMonitorResolution(); }
	};

}
#endif