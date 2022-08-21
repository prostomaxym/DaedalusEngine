#include "dlpch.h"
#include "Application.h"

#ifdef DL_PLATFORM_WINDOWS
	#include "Platform/Windows/WindowsWindow.h"
#endif

#include <GLFW/glfw3.h>

namespace Daedalus {

Daedalus::Application::Application()
{
#ifdef DL_PLATFORM_WINDOWS
	m_window = std::make_unique<WindowsWindow>(WindowProps());
#endif 

}

Daedalus::Application::~Application()
{
}

void Application::Run()
{
	while (m_running)
	{
		glClearColor(1, 0, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		m_window->OnUpdate();
	}
}

}