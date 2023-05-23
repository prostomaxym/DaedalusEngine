#pragma once

#include "UUID.h"
#include "Scene.h"
#include "Components.h"

#include "Daedalus/Core/Core.h"
#include "Daedalus/Debug/Log.h"

#include <entt/entt.hpp>

namespace Daedalus 
{
	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene);
		Entity(const Entity& other) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			DL_ASSERT(!HasComponent<T>(), Log::Categories::ECS, "Entity already has component!");
			T& component = m_scene->m_registry.emplace<T>(m_entity_handle, std::forward<Args>(args)...);
			m_scene->OnComponentAdded<T>(*this, component);
			return component;
		}

		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args)
		{
			T& component = m_scene->m_registry.emplace_or_replace<T>(m_entity_handle, std::forward<Args>(args)...);
			m_scene->OnComponentAdded<T>(*this, component);
			return component;
		}

		template<typename T>
		T& GetComponent()
		{
			DL_ASSERT(HasComponent<T>(), Log::Categories::ECS, "Entity does not have component!");
			return m_scene->m_registry.get<T>(m_entity_handle);
		}

		template<typename T>
		bool HasComponent()
		{
			return m_scene->m_registry.any_of<T>(m_entity_handle);
		}

		template<typename T>
		void RemoveComponent()
		{
			DL_ASSERT(HasComponent<T>(), Log::Categories::ECS, "Entity does not have component!");
			m_scene->m_registry.remove<T>(m_EntityHandle);
		}

		operator bool() const { return m_entity_handle != entt::null; }
		operator entt::entity() const { return m_entity_handle; }
		operator uint32_t() const { return (uint32_t)m_entity_handle; }

		UUID GetUUID() { return GetComponent<IDComponent>().ID; }
		const std::string& GetName() { return GetComponent<TagComponent>().tag; }

		bool operator==(const Entity& other) const
		{
			return m_entity_handle == other.m_entity_handle && m_scene == other.m_scene;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

	private:
		entt::entity m_entity_handle{ entt::null };
		Scene* m_scene = nullptr;
	};

}