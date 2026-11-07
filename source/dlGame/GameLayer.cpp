#include "GameLayer.h"
#include <Daedalus.h>

using namespace Daedalus;

void GameLayer::OnAttach()
{
	std::string error;
	if (!LuaScript::Register(m_scripts, Application::GetInstance()->GetEngine().GetAssets().GetFiles()))
	{
		Log::Write(Log::Levels::Error, Log::Categories::Application, "Failed to register scene scripts");
		return;
	}
	auto scene = std::make_unique<Scene>();
	if (!SceneSerializer::LoadAsset(Application::GetInstance()->GetEngine().GetAssets(), PathConfig::GetSceneAssetPath(), *scene, m_scripts, error))
	{
		Log::Write(Log::Levels::Error, Log::Categories::Application, "Scene load failed: {}", error);
		return;
	}
	Application::GetInstance()->GetEngine().GetScenes().RequestScene(std::move(scene));
}

void GameLayer::OnDetach()
{
	m_scripts = ScriptRegistry{};
}
