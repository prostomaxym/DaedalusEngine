#pragma once

#include "Macros.h"
#include "LayerStack.h"
#include <memory>

namespace Daedalus {
	class Engine;
	class Scene;
	class Window;
	class Event;
	class EventEngine;
	class WindowCloseEvent;
	class WindowResizeEvent;
	class ImGuiLayer;

	class DAEDALUS_API Application
	{
	public:
		static Application* GetInstance();
		virtual ~Application();

		Application(const Application&) = delete;
		Application(Application&&) = delete;
		Application& operator=(const Application&) = delete;
		Application& operator=(Application&&) = delete;

		Window& GetWindow();
		EventEngine& GetEventEngine();
		Engine& GetEngine();

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

		std::unique_ptr<Engine> m_engine;
		ImGuiLayer* m_debug_layer{ nullptr }; // LayerStack owns imgui_layer
		LayerStack m_layer_stack;

		bool m_running = true;
	};

	//Define in client
	Application* CreateApplication();
}