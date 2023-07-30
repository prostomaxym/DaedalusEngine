#include "dlpch.h"

#include "Scene.h"

#include "Daedalus/Renderer/API/Renderer.h"
#include "Entity.h"
#include "NativeScript.h"

using namespace Daedalus;

void Scene::OnRuntimeStart()
{
	m_is_running = true;
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
		const auto test_shader = Renderer::s_shader_library->Get("TestShader");

		RenderCommand::SetClearColor({ 0.2f, 0.2f, 0.2f, 1.f });
		RenderCommand::Clear();

		auto& main_camera = FindEntityByName("Main Camera").GetComponent<CameraComponent>().camera;
		Renderer::BeginScene(main_camera);

		test_shader->Bind();
		test_shader->SetFloat3("u_view_pos", main_camera.GetPosition());
		test_shader->SetFloat3("u_light_pos", glm::vec3(-60000.f, 100000.f, 80000.f));
		test_shader->SetFloat3("u_light_color", glm::vec3(1.f, 0.85f, 0.8f));
		test_shader->SetFloat3("u_ambient_strength", glm::vec3(0.1f, 0.1f, 0.1f));
		test_shader->SetFloat("u_spec_strength", 0.3f);
		test_shader->SetFloat("u_light_power", 175000.f);
		test_shader->SetFloat("u_shininess", 16.f);

		const auto view = m_registry.view<RenderableObjectComponent>();
		for (auto e : view)
		{
			Entity entity = { e, this };

			Renderer::Submit(test_shader.get(), &entity.GetComponent<RenderableObjectComponent>().model, entity.GetComponent<TransformComponent>().GetTransform());
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
