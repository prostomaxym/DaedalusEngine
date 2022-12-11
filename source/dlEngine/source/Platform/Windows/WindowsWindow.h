#pragma once

#ifdef DL_PLATFORM_WINDOWS

#include "Platform/GLFW/GLFWWindow.h"

namespace Daedalus {

class WindowsWindow : public GLFWWindow
{
public:
	WindowsWindow(const WindowProps& props);
	virtual ~WindowsWindow() = default;
};
}
#endif // DL_PLATFORM_WINDOWS