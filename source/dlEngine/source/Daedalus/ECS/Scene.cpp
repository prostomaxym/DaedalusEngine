#include "dlpch.h"

#include "Scene.h"

#include "Daedalus/Renderer/API/RenderConstants.h"
#include "Daedalus/Renderer/API/Renderer.h"
#include "Entity.h"
#include "NativeScript.h"

using namespace Daedalus;

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
	{
		const auto standard_shader = Renderer::s_shader_library->Get(ShaderConstants::StandardShader);

		RenderCommand::SetClearColor({ 0.2f, 0.2f, 0.2f, 1.f });
		RenderCommand::Clear();

		auto& main_camera = FindEntityByName("Main Camera").GetComponent<CameraComponent>().camera;
		Renderer::BeginScene(main_camera);

		UpdateDynamicLighting();
				
		main_camera.UpdatePositionUniform(standard_shader, ShaderConstants::SceneViewPos);

		const auto view = m_registry.view<RenderableObjectComponent>();
		for (auto e : view)
		{
			Entity entity = { e, this };

			Renderer::Submit(standard_shader.get(), &entity.GetComponent<RenderableObjectComponent>().model, entity.GetComponent<TransformComponent>().GetTransform());
		}

		Renderer::EndScene();
	}
}

void Scene::OnViewportResize(uint32_t width, uint32_t height)
{
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
	UpdateStaticLighting();
}

void Scene::UpdateStaticLighting()
{
	std::vector<LightUBO> light_UBOs;
	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	for (auto e : dir_view)
	{
		Entity entity = { e, this };
		const auto& light_component = entity.GetComponent<DirectionalLightComponent>();

		if (!light_component.is_dynamic)
			light_UBOs.emplace_back(light_component.light.GetShaderUBO());
	}

	const auto point_view = m_registry.view<PointLightComponent>();
	for (auto e : point_view)
	{
		Entity entity = { e, this };

		const auto& light_component = entity.GetComponent<PointLightComponent>();

		if (!light_component.is_dynamic)
			light_UBOs.emplace_back(light_component.light.GetShaderUBO());
	}

	const auto spot_view = m_registry.view<SpotLightComponent>();
	for (auto e : spot_view)
	{
		Entity entity = { e, this };
		const auto& light_component = entity.GetComponent<SpotLightComponent>();

		if (!light_component.is_dynamic)
			light_UBOs.emplace_back(light_component.light.GetShaderUBO());
	}

	Renderer::UpdateStaticLightUBO(light_UBOs);
}

void Scene::UpdateDynamicLighting()
{
	std::vector<LightUBO> light_UBOs;
	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	for (auto e : dir_view)
	{
		Entity entity = { e, this };
		const auto& light_component = entity.GetComponent<DirectionalLightComponent>();

		if (light_component.is_dynamic)
			light_UBOs.emplace_back(light_component.light.GetShaderUBO());
	}

	const auto point_view = m_registry.view<PointLightComponent>();
	for (auto e : point_view)
	{
		Entity entity = { e, this };
		const auto& light_component = entity.GetComponent<PointLightComponent>();

		if (light_component.is_dynamic)
			light_UBOs.emplace_back(light_component.light.GetShaderUBO());
	}

	const auto spot_view = m_registry.view<SpotLightComponent>();
	for (auto e : spot_view)
	{
		Entity entity = { e, this };
		const auto& light_component = entity.GetComponent<SpotLightComponent>();

		if (light_component.is_dynamic)
			light_UBOs.emplace_back(light_component.light.GetShaderUBO());
	}

	Renderer::UpdateDynamicLightUBO(light_UBOs);
}
