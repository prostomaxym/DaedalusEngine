#include "Platform.h"

#include "Linux/LinuxInput.h"
#include "Linux/LinuxWindow.h"
#include "Windows/WindowsInput.h"
#include "Windows/WindowsWindow.h"

using namespace Daedalus;

Input* Input::s_instance = nullptr;

std::unique_ptr<Window> Daedalus::Platform::createWindow(const WindowProps& props)
{
#ifdef DL_PLATFORM_WINDOWS
	return std::make_unique<WindowsWindow>(props);
#elif defined DL_PLATFORM_LINUX
	return std::make_unique<LinuxWindow>(props);
#else
	static_assert(false, "Unsupported Platfrom")
#endif
}

void Daedalus::Platform::InitInputSystem()
{
#ifdef DL_PLATFORM_WINDOWS
	Input::CreateInstance(new WindowsInput());
#elif defined DL_PLATFORM_LINUX
	Input::CreateInstance(new LinuxInput());
#else
	static_assert(false, "Unsupported Platfrom")
#endif
}

std::pair<int, int> Daedalus::Platform::GetMonitorResolution()
{
#ifdef DL_PLATFORM_WINDOWS
	return WindowsWindow::GetMonitorResolution();
#elif defined DL_PLATFORM_LINUX
	return LinuxWindow::GetMonitorResolution();
#else
	static_assert(false, "Unsupported Platfrom")
#endif
	
}
