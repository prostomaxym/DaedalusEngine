#include "dlpch.h"

#include "Scene.h"

#include "Daedalus/Core/Application.h"
#include "Entity.h"
#include "NativeScript.h"
#include "NativeScriptComponent.h"

using namespace Daedalus;

Scene::Scene() : m_render_system(m_registry, this) {}

void Scene::OnRuntimeStart()
{
	m_is_running = true;

	PrepareScene();
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

void Scene::PrepareScene()
{
    const auto& wnd = Application::GetInstance()->GetWindow();
    m_render_system.SetViewportSize(wnd.GetWidth(), wnd.GetHeight());
    m_render_system.SetCamera(&FindEntityByName("Main Camera").GetComponent<CameraComponent>().camera);
	m_render_system.UpdateStaticLighting();
}

#ifdef DL_PLATFORM_LINUX
template<typename T>
void Scene::OnComponentAdded(Entity entity, T& component)
{
    static_assert(sizeof(T) == 0);
}

template<>
void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component)
{
}

template<>
void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
{
}

template<>
void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
{
}

template<>
void Scene::OnComponentAdded<RenderableObjectComponent>(Entity entity, RenderableObjectComponent& component)
{
}

template<>
void Scene::OnComponentAdded<CubemapComponent>(Entity entity, CubemapComponent& component)
{
}

template<>
void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
{
}

template<>
void Scene::OnComponentAdded<DirectionalLightComponent>(Entity entity, DirectionalLightComponent& component)
{
}

template<>
void Scene::OnComponentAdded<PointLightComponent>(Entity entity, PointLightComponent& component)
{
}

template<>
void Scene::OnComponentAdded<SpotLightComponent>(Entity entity, SpotLightComponent& component)
{
}

template<>
void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
{
}
#endif
