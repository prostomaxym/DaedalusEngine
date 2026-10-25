#include "dlpch.h"

#include "Scene.h"

#include "Core/Application.h"
#include "Components.h"
#include "Entity.h"
#include "NativeScript.h"
#include "NativeScriptComponent.h"
#include "Physics/Physics.h"

using namespace Daedalus;

Scene::Scene() :
	m_render_system(m_registry, this)
	, m_physics_system(m_registry, this)
{
}

Scene::~Scene()
{
	Clear();
}

bool Scene::OnRuntimeStart(const std::string& scenename)
{
	auto* camera = GetCamera();
	if (m_is_running || !camera || !Application::GetInstance())
		return false;
	if (!scenename.empty()) m_scene_name = scenename;

	const auto& wnd = Application::GetInstance()->GetWindow();
	OnViewportResize(wnd.GetWidth(), wnd.GetHeight());
	m_render_system.SetViewportSize(wnd.GetWidth(), wnd.GetHeight());
	m_render_system.SetCamera(camera);
	m_render_system.OnStartRuntime();
	m_physics_system.OnStartRuntime();
	m_is_running = true;
	return true;
}

void Scene::OnRuntimeStop()
{
	if (m_is_running)
	{
		for (auto entity : m_registry.view<DirectionalLightComponent>())
			m_render_system.RemoveLight(&m_registry.get<DirectionalLightComponent>(entity).light);
		for (auto entity : m_registry.view<PointLightComponent>())
			m_render_system.RemoveLight(&m_registry.get<PointLightComponent>(entity).light);
		for (auto entity : m_registry.view<SpotLightComponent>())
			m_render_system.RemoveLight(&m_registry.get<SpotLightComponent>(entity).light);
	}
	m_is_running = false;
	m_render_system.SetCamera(nullptr);
}

void Scene::OnUpdateRuntime(DeltaTime dt)
{
	if (!m_is_running) return;
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
		m_physics_system.OnUpdateRuntime(dt);
	}

	// Graphics
    m_render_system.OnUpdateRuntime(dt);
}

void Scene::OnViewportResize(uint32_t width, uint32_t height)
{
	if (width == 0 || height == 0) return;
	for (auto entity : m_registry.view<CameraComponent>())
	{
		auto& component = m_registry.get<CameraComponent>(entity);
		if (component.viewport_aspect)
			if (auto* perspective = dynamic_cast<PerspectiveCamera*>(component.camera.get()))
				perspective->SetAspectRatio(static_cast<float>(width) / static_cast<float>(height));
	}
	if (m_is_running) m_render_system.SetViewportSize(width, height);
}

Entity Scene::CreateEntity(const std::string& name)
{
	return CreateEntityWithUUID(UUID(), name);
}

Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
{
	if (static_cast<uint64_t>(uuid) == 0 || m_entity_map.count(uuid)) return {};
	Entity entity(m_registry.create(), this);
	entity.AddComponent<IDComponent>(uuid);
	entity.AddComponent<TransformComponent>();
	auto& tag = entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);
	m_entity_map[uuid] = entity;

	return entity;
}

void Scene::DestroyEntity(Entity entity)
{
	if (!m_registry.valid(static_cast<entt::entity>(entity))) return;
	if (entity.HasComponent<DirectionalLightComponent>()) entity.RemoveComponent<DirectionalLightComponent>();
	if (entity.HasComponent<PointLightComponent>()) entity.RemoveComponent<PointLightComponent>();
	if (entity.HasComponent<SpotLightComponent>()) entity.RemoveComponent<SpotLightComponent>();
	if (entity.GetUUID() == m_primary_camera)
	{
		m_primary_camera = UUID(0);
		m_render_system.SetCamera(nullptr);
	}
	m_entity_map.erase(entity.GetUUID());
	m_registry.destroy(entity);
}

void Scene::Clear()
{
	OnRuntimeStop();
	while (!m_entity_map.empty()) DestroyEntity(Entity(m_entity_map.begin()->second, this));
	m_primary_camera = UUID(0);
	m_scene_name.clear();
	m_is_paused = false;
}

bool Scene::SetPrimaryCamera(UUID uuid)
{
	auto entity = GetEntityByUUID(uuid);
	if (!entity || !entity.HasComponent<CameraComponent>() || !entity.GetComponent<CameraComponent>().camera) return false;
	m_primary_camera = uuid;
	if (m_is_running) m_render_system.SetCamera(entity.GetComponent<CameraComponent>().camera.get());
	return true;
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
	auto ent = static_cast<uint64_t>(m_primary_camera) ? GetEntityByUUID(m_primary_camera) : FindEntityByName("Main Camera");

	if (!ent.IsValid() || !ent.HasComponent<CameraComponent>())
		return nullptr;

	const auto camera = ent.GetComponent<CameraComponent>().camera.get();

	return camera;
}