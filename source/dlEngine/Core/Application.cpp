#include "dlpch.h"
#include "Application.h"

#include "Macros.h"
#include "ResourceManager.h"
#include "Config/GraphicsConfig.h"
#include "Config/KeybindConfig.h"
#include "Config/PathConfig.h"
#include "Debug/ConsoleWindow.h"
#include "Debug/DebugOverlay.h"
#include "Events/EventDispatcher.h"
#include "Renderer/API/Renderer.h"
#include "Utils/FPSLocker.h"
#include "Utils/WorkingDirectory.h"

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

	Platform::SetupSegfaultHandler();	
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
	ResourceManager::Init();
	ResourceManager::LoadShaders(PathConfig::GetShadersPath(), GraphicsConfig::RecompilingShadersEnabled());

	auto imgui = std::make_unique<ImGuiLayer>();
	m_imgui_layer = imgui.get();
	PushOverlay(std::move(imgui));

	auto debug_overlay = std::make_unique<DebugOverlay>();
	if (IsDevBuild())
	{
		m_imgui_layer->AddWindow(std::move(debug_overlay));
	}
	
	auto console = std::make_unique<ConsoleWindow>();
	m_imgui_layer->AddWindow(std::move(console));
}

Application::~Application()
{
	WorkingDirectory::SaveConfigs();
	Input::Shutdown();
	Renderer::Shutdown();
	ResourceManager::Shutdown();
	Log::Shutdown();
}

void Application::Run()
{
	Timer update_timer;
	Timer frame_timer;

	while (m_running)
	{
		const auto update_dt = update_timer.UpdateDeltaTime();

		m_window->PollEvents();
		Input::Update();

		for (auto& layer : m_layer_stack)
		{
			layer->OnUpdate(update_dt);
		}

		m_window->SwapBuffers();

		const auto frame_dt = frame_timer.GetEllapsedTime();
		FPSLocker::LockFpsThreadSleep(GraphicsConfig::GetFPSLock(), frame_dt);
		frame_timer.StartTimer();
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

Scene* Application::GetMainScene()
{
	auto layer = m_layer_stack.GetMainLayer();

	return layer ? layer->GetScene() : nullptr;
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