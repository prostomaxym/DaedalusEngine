#pragma once

#ifdef DL_PLATFORM_WINDOWS

#include "Platform/GLFW/GLFWWindow.h"

namespace Daedalus {

class WindowsWindow final : public GLFWWindow
{
public:
	WindowsWindow(const WindowProps& props);
	~WindowsWindow() = default;
};
}
#endif // DL_PLATFORM_WINDOWS