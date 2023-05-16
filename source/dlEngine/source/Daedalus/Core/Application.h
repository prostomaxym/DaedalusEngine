#pragma once

#include "Core.h"
#include "Daedalus/Events/Event.h"
#include "Daedalus/Events/ApplicationEvent.h"
#include "Daedalus/UI/ImGui/ImGuiLayer.h"
#include "Window.h"
#include "LayerStack.h"

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

		void Run();

		void OnEvent(Event& event);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

	protected:
		Application();

		static Application* s_instance;

	private:
		bool OnWindowClosed(WindowCloseEvent& event);

		std::unique_ptr<Window> m_window;
		ImGuiLayer* m_imgui_layer; // LayerStack owns imgui_layer
		LayerStack m_layer_stack;

		bool m_running = true;
	};

	//Define in client
	Application* CreateApplication();
}



