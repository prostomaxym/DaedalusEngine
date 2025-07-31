#pragma once

#include "Macros.h"
#include "Window.h"
#include "LayerStack.h"

#include "Events/Event.h"
#include "Events/EventsEngine.h"
#include "Events/ApplicationEvent.h"
#include "Debug/ImGuiLayer.h"

namespace Daedalus {

	class DAEDALUS_API Application
	{
	public:
		static Application* GetInstance();
		virtual ~Application();

		Application(const Application&) = delete;
		Application(const Application&&) = delete;
		Application& operator=(const Application&) = delete;
		Application& operator=(const Application&&) = delete;

		inline Window& GetWindow() { return *m_window; }
		EventEngine& GetEventEngine() { return m_event_engine; }

		void Run();

		void OnEvent(Event& event);

		void PushLayer(std::unique_ptr<Layer>&& layer);
		void PushOverlay(std::unique_ptr<Layer>&& overlay);

		Scene* GetMainScene(); //Use it only for debugging

	protected:
		Application();

		static Application* s_instance;

	private:
		bool OnWindowClosed(WindowCloseEvent& event);
		bool OnWindowResized(WindowResizeEvent& event);
	
		std::unique_ptr<Window> m_window;
		ImGuiLayer* m_imgui_layer{ nullptr };
		LayerStack m_layer_stack;
		EventEngine m_event_engine;

		bool m_running = true;
	};

	//Define in client
	Application* CreateApplication();
}



