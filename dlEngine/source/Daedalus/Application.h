#pragma once

#include "Core.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "IWindow.h"

namespace Daedalus {

class DAEDALUS_API Application
{
public:
	Application();
	virtual ~Application();

	void Run();

	void OnEvent(Event& event);

private:
	bool OnWindowClosed(WindowCloseEvent& event);

	std::unique_ptr<IWindow> m_window;
	bool m_running = true;
};

//Define in client
Application* CreateApplication();
}



