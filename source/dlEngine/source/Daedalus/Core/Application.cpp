#include "dlpch.h"
#include "Application.h"

#include "Core.h"
#include "Daedalus/Config/GraphicsConfig.h"
#include "Daedalus/Events/EventDispatcher.h"
#include "Daedalus/Renderer/API/Renderer.h"
#include "Daedalus/Utils/FPSLocker.h"
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

Application::Application()
{
	s_instance = this;
	WorkingDirectory::LoadConfigs();
	
	WindowProps props;
	props.title = GraphicsConfig::GetWindowTitle();
	props.width = GraphicsConfig::GetWindowWidth();
	props.height = GraphicsConfig::GetWindowHeight();
	props.vsync = GraphicsConfig::IsVSyncEnabled();
	props.screenmode = GraphicsConfig::IsWindowFullscreen() ? WindowProps::ScreenMode::windowed_fullscreen : WindowProps::ScreenMode::windowed;
	m_window = Platform::createWindow(props);
	
	m_event_engine.Start();
	Platform::InitInputSystem();
	m_window->SetEventCallback(DL_BIND_EVENT_FN(Application::OnEvent));

	Renderer::Init();
	Renderer::SetupGraphicSettings();
	Renderer::LoadShaderLibrary(WorkingDirectory::GetShaderDirectory(), true);

	//std::unique_ptr<ImGuiLayer> lay = std::make_unique<ImGuiLayer>();
	//m_imgui_layer = lay.get();
	//PushOverlay(std::move(lay));
}

Application::~Application()
{
	WorkingDirectory::SaveConfigs();
	Input::Shutdown();
	Renderer::Shutdown();
}

void Application::Run()
{
	Timer timer;

	while (m_running)
	{	
		for (auto& layer : m_layer_stack)
		{
			layer->OnUpdate(timer.GetEllapsedTime());
		}

		timer.StartTimer();

		//m_imgui_layer->Begin();
		//m_imgui_layer->End();

		m_window->OnUpdate();

		FPSLocker::LockFps(GraphicsConfig::GetFPSLock(), timer.GetEllapsedTime());
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

void Application::PushLayer(std::unique_ptr<Layer>&& layer)
{
	m_layer_stack.PushLayer(std::move(layer));
}

void Application::PushOverlay(std::unique_ptr<Layer>&& overlay)
{
	m_layer_stack.PushOverlay(std::move(overlay));
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
