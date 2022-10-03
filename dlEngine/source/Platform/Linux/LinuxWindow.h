#pragma once

#ifdef DL_PLATFORM_LINUX

#include "Platform/GLFW/GLFWWindow.h"

namespace Daedalus {

class LinuxWindow : public GLFWWindow
{
public:
	LinuxWindow(const WindowProps& props);
	virtual ~LinuxWindow() = default;
};

}
#endif