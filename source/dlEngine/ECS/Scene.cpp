#include "dlpch.h"

#include "Scene.h"

#include "Core/Application.h"
#include "Components.h"
#include "Entity.h"
#include "NativeScript.h"
#include "NativeScriptComponent.h"

using namespace Daedalus;

Scene::Scene() : m_render_system(m_registry, this) {}

void Scene::OnRuntimeStart(const std::string& scenename)
{
	m_scene_name = scenename;
	m_is_running = true;

	const auto& wnd = Application::GetInstance()->GetWindow();
	m_render_system.SetViewportSize(wnd.GetWidth(), wnd.GetHeight());
	m_render_system.SetCamera(FindEntityByName("Main Camera").GetComponent<CameraComponent>().camera.get());
	m_render_system.OnStartRuntime();
}

void Scene::OnRuntimeStop()
{
	m_is_running = false;
}

void Scene::OnUpdateRuntime(DeltaTime dt)
{
	if (!m_is_paused)
	{
		// Update scripts
		{
			m_registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
				{
					for(auto& script : nsc.native_scripts)
						script.second->OnUpdate(dt);
				});
		}

		// Physics
		{

		}
	}

	// Graphics
    m_render_system.OnUpdateRuntime(dt);
}

void Scene::OnViewportResize(uint32_t width, uint32_t height)
{
    m_render_system.SetViewportSize(width, height);
}

Entity Scene::CreateEntity(const std::string& name)
{
	return CreateEntityWithUUID(UUID(), name);
}

Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
{
	Entity entity(m_registry.create(), this);
	entity.AddComponent<IDComponent>(uuid);
	entity.AddComponent<TransformComponent>();
	auto& tag = entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);
	m_entity_map[uuid] = entity;

	return entity;
}

void Scene::DestroyEntity(Entity entity)
{
	m_entity_map.erase(entity.GetUUID());
	m_registry.destroy(entity);
}

Entity Scene::FindEntityByName(std::string_view name)
{
	auto view = m_registry.view<TagComponent>();
	for (auto entity : view)
	{
		const TagComponent& tc = view.get<TagComponent>(entity);
		if (tc.tag == name)
			return Entity(entity, this);
	}

	return {};
}

Entity Scene::GetEntityByUUID(UUID uuid)
{
	if (m_entity_map.find(uuid) != m_entity_map.end())
		return { m_entity_map.at(uuid), this };

	return {};
}

Camera* Scene::GetCamera()
{
	auto ent = FindEntityByName("Main Camera");

	if (!ent.IsValid())
		return nullptr;

	const auto camera = ent.GetComponent<CameraComponent>().camera.get();

	return camera;
}