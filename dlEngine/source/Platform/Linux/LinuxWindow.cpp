#ifdef DL_PLATFORM_LINUX

#include "dlpch.h"
#include "LinuxWindow.h"

namespace Daedalus {

LinuxWindow::LinuxWindow(const WindowProps& props) : GLFWWindow(props) {}

}

#endif // DL_PLATFORM_LINUX