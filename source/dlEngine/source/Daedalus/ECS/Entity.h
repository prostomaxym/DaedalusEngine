#pragma once

#include "UUID.h"
#include "Scene.h"
#include "Components.h"

#include "Daedalus/Core/Core.h"
#include "Daedalus/Debug/Log.h"

#include <entt/entt.hpp>

namespace Daedalus
{
	class DAEDALUS_API Entity
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
			m_scene->OnComponentAdded<T>(component);
			return component;
		}

		template<typename T>
		void PushComponent(T&& component)
		{
			DL_ASSERT(!HasComponent<T>(), Log::Categories::ECS, "Entity already has component!");
			T& comp = m_scene->m_registry.emplace<T>(m_entity_handle, std::move(component));
			m_scene->OnComponentAdded<T>(component);
		}

		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args)
		{
			T& component = m_scene->m_registry.emplace_or_replace<T>(m_entity_handle, std::forward<Args>(args)...);
			m_scene->OnComponentAdded<T>(component);
			return component;
		}

		template<typename T>
		T& GetComponent() const
		{
			DL_ASSERT(HasComponent<T>(), Log::Categories::ECS, "Entity does not have component!");
			return m_scene->m_registry.get<T>(m_entity_handle);
		}

		template<typename T>
		bool HasComponent() const
		{
			return m_scene->m_registry.any_of<T>(m_entity_handle);
		}

		template<typename T>
		void RemoveComponent()
		{
			auto& component = GetComponent<T>();
			m_scene->OnComponentRemoved<T>(component);
			m_scene->m_registry.remove<T>(m_entity_handle);
		}

		template<typename T>
		std::unique_ptr<T> PopComponent()
		{
			DL_ASSERT(HasComponent<T>(), Log::Categories::ECS, "Entity does not have component!");
			auto component = std::make_unique<T>(std::move(m_scene->m_registry.get<T>(m_entity_handle)));
			m_scene->m_registry.remove<T>(m_entity_handle);
			return component;
		}

		template<typename T>
		void PushComponent(std::unique_ptr<T>&& component)
		{
			auto comp_inst = *component.release();
			DL_ASSERT(!HasComponent<T>(), Log::Categories::ECS, "Entity already has component!");
			T& comp = m_scene->m_registry.emplace<T>(m_entity_handle, std::move(comp_inst));
			m_scene->OnComponentAdded<T>(comp);
		}

		operator bool() const { return m_entity_handle != entt::null; }
		operator entt::entity() const { return m_entity_handle; }
		operator uint32_t() const { return (uint32_t)m_entity_handle; }

		UUID GetUUID() { return GetComponent<IDComponent>().ID; }
		const std::string& GetName() const { return GetComponent<TagComponent>().tag; }

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
		Scene* m_scene{ nullptr };
	};
}
