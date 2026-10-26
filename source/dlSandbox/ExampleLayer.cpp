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
	auto scene = std::make_unique<Scene>();
	if (!SceneSerializer::Load(PathConfig::GetScenePath(), *scene, assets, m_scripts, error))
	{
		Log::Write(Log::Levels::Error, Log::Categories::Application, "Scene load failed: {}", error);
		return;
	}
	Application::GetInstance()->GetEngine().GetScenes().RequestScene(std::move(scene));
}

void ExampleLayer::OnDetach()
{
	m_scripts = ScriptRegistry{};
}
