#pragma once

#include "Core.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "IWindow.h"
#include "LayerStack.h"

namespace Daedalus {

class DAEDALUS_API Application
{
public:
	static Application* GetInstance();

	inline IWindow& GetWindow() { return *m_window; }

	void Run();

	void OnEvent(Event& event);

	void PushLayer(Layer* layer);
	void PushOverlay(Layer* overlay);

protected:
	Application();

	static Application* s_instance;

private:
	bool OnWindowClosed(WindowCloseEvent& event);

	std::unique_ptr<IWindow> m_window;
	LayerStack m_layer_stack;
	bool m_running = true;
};

//Define in client
Application* CreateApplication();
}



