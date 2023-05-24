#include "Platform.h"

#include "Linux/LinuxInput.h"
#include "Linux/LinuxWindow.h"
#include "Windows/WindowsInput.h"
#include "Windows/WindowsWindow.h"

using namespace Daedalus;

Input* Input::s_instance = nullptr;

std::unique_ptr<Window> Daedalus::Platform::createWindow()
{
#ifdef DL_PLATFORM_WINDOWS
	return std::make_unique<WindowsWindow>(WindowProps());
#elif defined DL_PLATFORM_LINUX
	return std::make_unique<LinuxWindow>(WindowProps());
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
