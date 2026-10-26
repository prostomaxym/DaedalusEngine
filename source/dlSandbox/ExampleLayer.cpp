#include "ExampleLayer.h"
#include "SceneScripts.h"

using namespace Daedalus;

void ExampleLayer::OnAttach()
{
	std::string error;
	if (!RegisterSceneScripts(m_scripts))
	{
		Log::Write(Log::Levels::Error, Log::Categories::Application, "Failed to register scene scripts");
		return;
	}
	const auto assets = PathConfig::GetAssetsPath();
	if (!SceneSerializer::Load(assets / m_scene_file, m_scene, assets, m_scripts, error))
	{
		Log::Write(Log::Levels::Error, Log::Categories::Application, "Scene load failed: {}", error);
		return;
	}
	if (!m_scene.OnRuntimeStart())
	{
		Log::Write(Log::Levels::Error, Log::Categories::Application, "Loaded scene has no usable primary camera");
		m_scene.Clear();
	}
}

void ExampleLayer::OnDetach()
{
	m_scene.Clear();
	m_scripts = ScriptRegistry{};
}

void ExampleLayer::OnUpdate(DeltaTime dt)
{
	m_scene.OnUpdateRuntime(dt);
}