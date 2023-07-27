#include "dlpch.h"
#include "Application.h"

#include "Core.h"
#include "Daedalus/Events/EventDispatcher.h"
#include "Daedalus/Renderer/API/Renderer.h"
#include "Daedalus/Utils/WorkingDirectory.h"

#include "Platform/Platform.h"

using namespace Daedalus;

Application* Application::s_instance = nullptr;

Application* Application::GetInstance()
{
	if (!s_instance)
	{
		s_instance = new Application();
	}

	return s_instance;
}

Application::Application() :
	m_window(Platform::createWindow())
{
	s_instance = this;

	m_event_engine.Start();
	Platform::InitInputSystem();
	m_window->SetEventCallback(DL_BIND_EVENT_FN(Application::OnEvent));

	Renderer::Init();
	Renderer::LoadShaderLibrary(WorkingDirectory::GetShaderDirectory());

	m_imgui_layer = new ImGuiLayer();
	PushOverlay(m_imgui_layer);
}

Application::~Application()
{
	Renderer::Shutdown();
}

void Application::Run()
{
	while (m_running)
	{
		for (auto layer : m_layer_stack)
		{
			layer->OnUpdate();
		}

		m_imgui_layer->Begin();
		m_imgui_layer->End();

		m_window->OnUpdate();
	}
}

void Application::OnEvent(Event& event)
{
	EventDispatcher::ProcessEvent<WindowCloseEvent>(event, DL_BIND_EVENT_FN(Application::OnWindowClosed));
	EventDispatcher::ProcessEvent<WindowResizeEvent>(event, DL_BIND_EVENT_FN(Application::OnWindowResized));

	Log::Write(Log::Levels::Trace, Log::Categories::Events, "{0}", event);

	for (auto it = m_layer_stack.end(); it != m_layer_stack.begin();)
	{
		if (event.IsHandled())
			break;

		(*--it)->OnEvent(event);
	}
}

void Application::PushLayer(Layer* layer)
{
	m_layer_stack.PushLayer(layer);
}

void Application::PushOverlay(Layer* overlay)
{
	m_layer_stack.PushOverlay(overlay);
}

bool Application::OnWindowClosed(WindowCloseEvent& event)
{
	m_running = false;
	return true;
}

bool Application::OnWindowResized(WindowResizeEvent& event)
{
	RenderCommand::SetViewport(0,0, event.GetWidth(), event.GetHeight());

	return true;
}
