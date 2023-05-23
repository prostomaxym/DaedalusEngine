#pragma once

#include "UUID.h"

#include <entt/entt.hpp>

namespace Daedalus
{
	class Entity;

	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
		Entity DuplicateEntity(Entity entity);
		void DestroyEntity(Entity entity);

		Entity FindEntityByName(std::string_view name);
		Entity GetEntityByUUID(UUID uuid);

	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

	private:
		entt::registry m_registry;
		std::unordered_map<UUID, entt::entity> m_entity_map;

		friend class Entity;
	};
}