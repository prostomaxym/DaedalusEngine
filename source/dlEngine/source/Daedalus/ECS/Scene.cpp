#include "dlpch.h"

#include "Scene.h"

#include "Daedalus/Renderer/API/Renderer.h"
#include "Entity.h"
#include "ScriptableEntity.h"

using namespace Daedalus;

void Scene::OnRuntimeStart()
{
	m_is_running = true;
}

void Scene::OnRuntimeStop()
{
	m_is_running = false;
}

void Scene::OnUpdateRuntime(DeltaTime ts)
{
	if (!m_is_paused)
	{
		// Update scripts
		{
			m_registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
				{
					if (!nsc.instance)
					{
						nsc.instance = nsc.InstantiateScript();
						nsc.instance->m_entity = Entity{ entity, this };
						nsc.instance->OnCreate();
					}

					nsc.instance->OnUpdate(ts);
				});
		}

		// Physics
		{

		}
	}

	// Graphics

	auto test_shader = Renderer::s_shader_library->Get("TestShader");

	RenderCommand::SetClearColor({ 0.2f, 0.2f, 0.2f, 1.f });
	RenderCommand::Clear();

	Renderer::BeginScene(FindEntityByName("Main Camera").GetComponent<CameraComponent>().camera);

	for (auto& handle : m_entity_map)
	{
		const auto entity = GetEntityByUUID(handle.first);

		if (entity.HasComponent<RenderableObjectComponent>())
		{
			Renderer::Submit(test_shader.get(), &entity.GetComponent<RenderableObjectComponent>().model, entity.GetComponent<TransformComponent>().GetTransform());
		}
	}
	
	Renderer::EndScene();

	test_shader->Unbind();

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
