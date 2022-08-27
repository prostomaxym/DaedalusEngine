#pragma once

#include "Core.h"
#include "IWindow.h"

namespace Daedalus {

class DAEDALUS_API Application
{
public:
	Application();
	virtual ~Application();

	void Run();

private:
	std::unique_ptr<IWindow> m_window;
	bool m_running = true;
};

//Define in client
Application* CreateApplication();
}



