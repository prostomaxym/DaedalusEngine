#include "dlpch.h"
#include "Application.h"

#ifdef DL_PLATFORM_WINDOWS
	#include "Platform/Windows/WindowsWindow.h"
#endif

#include "Daedalus/Events/EventDispatcher.h"

#include <GLFW/glfw3.h>

namespace Daedalus {
#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

Application::Application()
{
#ifdef DL_PLATFORM_WINDOWS
	m_window = std::make_unique<WindowsWindow>(WindowProps());
#endif 

	m_window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

}

Application::~Application()
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

void Application::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClosed));

	DL_CORE_TRACE("{0}", event);
}

bool Application::OnWindowClosed(WindowCloseEvent& event)
{
	m_running = false;
	return true;
}

}