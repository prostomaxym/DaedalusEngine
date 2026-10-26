#include "dlpch.h"
#include "Application.h"
#include "Engine.h"
#include "ECS/SceneManager.h"

#include "Macros.h"
#include "Config/GraphicsConfig.h"
#include "Config/KeybindConfig.h"
#include "Config/PathConfig.h"
#include "Debug/DebugOverlay.h"
#include "Debug/ConsoleOverlay.h"
#include "Events/EventDispatcher.h"
#include "Renderer/API/Renderer.h"
#include "Renderer/Resources/ResourceManager.h"
#include "Physics/Physics.h"
#include "Utils/FPSLocker.h"
#include "Utils/FixedStepClock.h"
#include "Utils/WorkingDirectory.h"

#include "Platform/Platform.h"

using namespace Daedalus;

Application* Application::s_instance = nullptr;

Application* Application::GetInstance()
{
	// Instance is created by the client (see CreateApplication); the base class is never instantiated lazily
	return s_instance;
}

Application::Application()
{
	Platform::SetupSegfaultHandler();
	DL_ASSERT(!s_instance, Log::Categories::EngineCore, "Only one Application may be active");
	if (s_instance) std::abort();
	s_instance = this;
	m_engine = std::make_unique<Engine>();
	GetWindow().SetEventCallback([this](Event& event) { GetEventEngine().AddEvent(event.Clone()); });

	auto debug_layer = std::make_unique<ImGuiLayer>();
	m_debug_layer = debug_layer.get();
	PushOverlay(std::move(debug_layer));

	if (IsDevBuild())
	{
		m_debug_layer->AddWindow(std::make_unique<DebugOverlay>());
		m_debug_layer->AddWindow(std::make_unique<ConsoleOverlay>());
	}
}

Application::~Application()
{
	m_engine->Stop();

	// Reverse of construction order; layers own scene/GPU resources so they go first
	m_layer_stack.Clear();
	m_debug_layer = nullptr;
	
	m_engine.reset();
	s_instance = nullptr;
}

Window& Application::GetWindow() { return m_engine->GetWindow(); }
EventEngine& Application::GetEventEngine() { return m_engine->GetEvents(); }
Engine& Application::GetEngine() { return *m_engine; }

void Application::Run()
{
	Timer update_timer;
	Timer frame_timer;
	FixedStepClock fixed_clock;

	while (m_running)
	{
		const auto update_dt = update_timer.UpdateDeltaTime();

		GetWindow().PollEvents();
		Input::Update();
		GetEventEngine().DispatchPending(DL_BIND_EVENT_FN(Application::OnEvent));
		if (!m_running) break;
		auto& scenes = m_engine->GetScenes();
		if (scenes.HasPending())
		{
			scenes.CommitPending();
			ResourceManager::PurgeUnused();
		}
		const int steps = fixed_clock.Advance(update_dt);
		for (int step = 0; step < steps; ++step)
		{
			scenes.OnFixedUpdate(DeltaTime(FixedStepClock::Step));
			for (auto& layer : m_layer_stack)
				layer->OnFixedUpdate(DeltaTime(FixedStepClock::Step));
		}

		for (auto& layer : m_layer_stack)
		{
			layer->OnUpdate(update_dt);
		}
		scenes.OnUpdate(update_dt);
		scenes.OnRender(update_dt);
		for (auto& layer : m_layer_stack)
			layer->OnRender(update_dt);
		m_debug_layer->BeginFrame();
		for (auto& layer : m_layer_stack)
			layer->OnImGuiRender(update_dt);
		m_debug_layer->EndFrame();

		GetWindow().SwapBuffers();

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
	return m_engine->GetScenes().GetActiveScene();
}

bool Application::OnWindowClosed(WindowCloseEvent& event)
{
	m_running = false;
	return true;
}

bool Application::OnWindowResized(WindowResizeEvent& event)
{
	if (event.GetWidth() && event.GetHeight())
	{
		Renderer::OnWindowResize(event.GetWidth(), event.GetHeight());
		m_engine->GetScenes().OnViewportResize(event.GetWidth(), event.GetHeight());
	}
	return false;
}