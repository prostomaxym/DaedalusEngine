#ifdef DL_PLATFORM_WINDOWS

#include "dlpch.h"
#include "WindowsWindow.h"

namespace Daedalus {

WindowsWindow::WindowsWindow(const WindowProps& props) : GLFWWindow(props) {}

}

#endif // DL_PLATFORM_WINDOWS