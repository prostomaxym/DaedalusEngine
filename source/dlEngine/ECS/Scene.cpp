#include "dlpch.h"

#include "Scene.h"

#include "Core/Application.h"
#include "Core/Window.h"
#include "Debug/Profiler.h"
#include "Components.h"
#include "Entity.h"
#include "NativeScript.h"
#include "NativeScriptComponent.h"
#include "Physics/Physics.h"
#include "Events/PhysicsEvent.h"
#include "Events/EventsEngine.h"
#include "Renderer/API/Renderer.h"
#include "Systems/RenderSystem.h"
#include "Systems/PhysicsSystem.h"
#include <algorithm>

using namespace Daedalus;

Scene::Scene() :
	m_render_system(std::make_unique<RenderSystem>(m_registry, this))
	, m_physics_system(std::make_unique<PhysicsSystem>(m_registry, this))
	, m_script_events(std::make_unique<EventEngine>(256))
{
	m_script_events->Start();
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
	m_render_system->SetViewportSize(wnd.GetWidth(), wnd.GetHeight());
	m_render_system->SetCamera(camera);
	m_render_system->OnStartRuntime();
	m_physics_system->OnStartRuntime();
	m_is_running = true;
	return true;
}

void Scene::OnRuntimeStop()
{
	for (auto handle : m_registry.view<NativeScriptComponent>())
		for (auto& [type, script] : m_registry.get<NativeScriptComponent>(handle).native_scripts)
			if (script) script->OnRuntimeStop();
	m_script_events->CleanQueue();
	if (m_is_running)
	{
		for (auto entity : m_registry.view<DirectionalLightComponent>())
			m_render_system->RemoveLight(&m_registry.get<DirectionalLightComponent>(entity).light);
		for (auto entity : m_registry.view<PointLightComponent>())
			m_render_system->RemoveLight(&m_registry.get<PointLightComponent>(entity).light);
		for (auto entity : m_registry.view<SpotLightComponent>())
			m_render_system->RemoveLight(&m_registry.get<SpotLightComponent>(entity).light);
	}
	m_physics_system->OnStopRuntime();
	m_is_running = false;
	m_render_system->SetCamera(nullptr);
}

void Scene::OnUpdateRuntime(DeltaTime dt)
{
	DL_PROFILE_SCOPE("Scene::Update");
	if (m_is_running && !m_is_paused) UpdateScripts(dt);
}

bool Scene::QueueScriptEvent(std::unique_ptr<Event> event)
{
	if (const auto* physics = dynamic_cast<const ScenePhysicsEvent*>(event.get());
		physics && physics->GetScene() != static_cast<uint64_t>(m_physics_event_source)) return false;
	return m_script_events->AddEvent(std::move(event));
}

void Scene::UpdateScripts(DeltaTime dt)
{
	if (m_is_iterating || m_is_constructing) return;
	m_is_iterating = true;
	m_script_events->DispatchPending([this](Event& event)
	{
		for (auto handle : m_registry.view<NativeScriptComponent>())
			for (auto& [type, script] : m_registry.get<NativeScriptComponent>(handle).native_scripts)
				if (script) script->OnEvent(event);
	});
	{
		DL_PROFILE_SCOPE("Scene::Scripts");
		for (auto handle : m_registry.view<NativeScriptComponent>())
			for (auto& [type, script] : m_registry.get<NativeScriptComponent>(handle).native_scripts)
				if (script) script->OnUpdate(dt);
	}
	m_is_iterating = false;
}

void Scene::OnFixedUpdateRuntime(DeltaTime dt)
{
	DL_PROFILE_SCOPE("Scene::FixedUpdate");
	if (m_is_running && !m_is_paused) StepPhysics(dt);
}

void Scene::StepPhysics(DeltaTime dt)
{
	if (m_is_iterating) return;
	m_is_iterating = true;
	m_physics_system->OnUpdateRuntime(dt);
	m_is_iterating = false;
}

void Scene::OnRenderRuntime(DeltaTime dt)
{
	DL_PROFILE_SCOPE("Scene::Render");
	m_is_iterating = true;
	if (m_is_running && GetCamera()) m_render_system->OnUpdateRuntime(dt);
#ifdef DL_DEV_TOOLS
	if (m_is_running && m_physics_debug_draw)
		GetPhysicsWorld().DrawDebug([](const auto& start, const auto& end, const auto& color)
		{
			(void)Renderer::DebugLine(start, end, color);
		});
#endif
	m_is_iterating = false;
}

void Scene::OnViewportResize(uint32_t width, uint32_t height)
{
	if (width == 0 || height == 0) return;
	m_viewport_width = width;
	m_viewport_height = height;
	for (auto entity : m_registry.view<CameraComponent>())
	{
		auto& component = m_registry.get<CameraComponent>(entity);
		if (component.viewport_aspect)
			if (auto* perspective = dynamic_cast<PerspectiveCamera*>(component.camera.get()))
				perspective->SetAspectRatio(static_cast<float>(width) / static_cast<float>(height));
	}
	if (m_is_running) m_render_system->SetViewportSize(width, height);
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
	m_transform_nodes.try_emplace(uuid);
	if (m_transform_order.capacity() < m_transform_nodes.size())
		m_transform_order.reserve(m_transform_nodes.size() * 2);
	m_hierarchy_dirty = true;

	return entity;
}

void Scene::DestroyEntity(Entity entity)
{
	if (entity.m_scene != this || !m_registry.valid(static_cast<entt::entity>(entity))) return;
	const auto destroyed_id = std::to_string(static_cast<uint64_t>(entity.GetUUID()));
	for (auto handle : m_registry.view<JointComponent>())
	{
		auto& component = m_registry.get<JointComponent>(handle);
		if (std::erase_if(component.description, [&](const auto& joint) { return joint.target == destroyed_id; }) != 0)
			component.joints.clear();
	}
	if (entity.HasComponent<DirectionalLightComponent>()) entity.RemoveComponent<DirectionalLightComponent>();
	if (entity.HasComponent<PointLightComponent>()) entity.RemoveComponent<PointLightComponent>();
	if (entity.HasComponent<SpotLightComponent>()) entity.RemoveComponent<SpotLightComponent>();
	if (entity.GetUUID() == m_primary_camera)
	{
		m_primary_camera = UUID(0);
		m_render_system->SetCamera(nullptr);
	}
	RemoveFromHierarchy(entity.GetUUID());
	m_prefab_origins.erase(entity.GetUUID());
	m_entity_map.erase(entity.GetUUID());
	m_registry.destroy(entity);
}

void Scene::SetPhysicsEventCallback(std::function<void(const ScenePhysicsEvent&)> callback)
{
	m_physics_callback = callback ? std::make_shared<std::function<void(const ScenePhysicsEvent&)>>(std::move(callback)) : nullptr;
}

Daedalus::UUID Scene::GetPhysicsEntity(uint64_t body) const
{
	if (!body) return UUID(0);
	for (auto handle : m_registry.view<RigidBodyComponent>())
		if (m_registry.get<RigidBodyComponent>(handle).handle == body) return m_registry.get<IDComponent>(handle).ID;
	for (auto handle : m_registry.view<KinematicBodyComponent>())
		if (m_registry.get<KinematicBodyComponent>(handle).handle == body) return m_registry.get<IDComponent>(handle).ID;
	for (auto handle : m_registry.view<CharacterControllerComponent>())
		if (m_registry.get<CharacterControllerComponent>(handle).handle == body) return m_registry.get<IDComponent>(handle).ID;
	return UUID(0);
}

std::span<const PrefabOrigin> Scene::GetPrefabOrigins(UUID entity) const
{
	const auto found = m_prefab_origins.find(entity);
	return found == m_prefab_origins.end() ? std::span<const PrefabOrigin>{} : std::span<const PrefabOrigin>(found->second);
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
	if (m_is_running) m_render_system->SetCamera(entity.GetComponent<CameraComponent>().camera.get());
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
	UpdateWorldTransforms();
	auto ent = static_cast<uint64_t>(m_primary_camera) ? GetEntityByUUID(m_primary_camera) : FindEntityByName("Main Camera");

	if (!ent.IsValid() || !ent.HasComponent<CameraComponent>())
		return nullptr;

	const auto camera = ent.GetComponent<CameraComponent>().camera.get();

	return camera;
}

PhysicsWorld& Scene::GetPhysicsWorld() { return m_physics_system->GetWorld(); }
