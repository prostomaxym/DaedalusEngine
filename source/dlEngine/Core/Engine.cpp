#include "dlpch.h"
#include "Engine.h"
#include "Renderer/Resources/ResourceManager.h"
#include "Input.h"
#include "Window.h"
#include "Config/GraphicsConfig.h"
#include "Config/KeybindConfig.h"
#include "Config/PathConfig.h"
#include "Events/EventsEngine.h"
#include "ECS/SceneManager.h"
#include "Physics/Physics.h"
#include "Platform/Platform.h"
#include "Renderer/API/Renderer.h"
#include "Utils/WorkingDirectory.h"

using namespace Daedalus;

namespace
{
	struct ConfigurationSession
	{
		PathConfig m_paths;
		GraphicsConfig m_graphics;
		KeybindConfig m_keybinds;
		ConfigurationSession() { WorkingDirectory::LoadConfigs(); }
		~ConfigurationSession() { WorkingDirectory::SaveConfigs(); }
	};
}

struct Engine::State
{
	// Declaration order preserves the context until every GPU resource is destroyed.
	ConfigurationSession m_configuration;
	std::unique_ptr<Input> m_input;
	std::unique_ptr<Window> m_window;
	std::unique_ptr<ResourceManager> m_resources;
	std::unique_ptr<Renderer> m_renderer;
	Physics m_physics;
	SceneManager m_scenes;
	EventEngine m_events;

	State()
	{
		m_input = Platform::CreateInputSystem();
		WindowProps props;
		props.title = GraphicsConfig::GetWindowTitle();
		props.width = GraphicsConfig::GetWindowWidth();
		props.height = GraphicsConfig::GetWindowHeight();
		props.vsync = GraphicsConfig::IsVSyncEnabled();
		props.screenmode = GraphicsConfig::IsWindowFullscreen() ? WindowProps::ScreenMode::windowed_fullscreen : WindowProps::ScreenMode::windowed;
		m_window = Platform::createWindow(props);
		m_resources = std::make_unique<ResourceManager>();
		m_renderer = std::make_unique<Renderer>();
		ResourceManager::LoadShaders(PathConfig::GetShadersPath(), GraphicsConfig::RecompilingShadersEnabled());
		m_events.Start();
	}

	~State()
	{
		m_events.Stop();
		m_scenes.Clear();
	}
};

Engine::Engine() : m_state(std::make_unique<State>()) {}
Engine::~Engine() = default;
Window& Engine::GetWindow() { return *m_state->m_window; }
EventEngine& Engine::GetEvents() { return m_state->m_events; }
SceneManager& Engine::GetScenes() { return m_state->m_scenes; }
void Engine::Stop()
{
	m_state->m_events.Stop();
	m_state->m_scenes.Clear();
}
