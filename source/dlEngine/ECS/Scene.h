#pragma once

#include "UUID.h"
#include "Systems/RenderSystem.h"
#include "Systems/PhysicsSystem.h"
#include "Utils/DeltaTime.h"
#include "Components.h"

#include <entt/entt.hpp>

namespace Daedalus
{
	class Entity;
	class SceneSerializer;
	struct NativeScriptComponent;

	class DAEDALUS_API Scene
	{
	public:
		Scene();
		~Scene();
		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;
		Scene(Scene&&) = delete;
		Scene& operator=(Scene&&) = delete;

		bool OnRuntimeStart(const std::string& scenename = "");
		void OnRuntimeStop();
		void OnUpdateRuntime(DeltaTime dt);

		void OnViewportResize(uint32_t width, uint32_t height);

		bool IsRunning() const { return m_is_running; }
		bool IsPaused() const { return m_is_paused; }

		void SetPaused(bool paused) { m_is_paused = paused; }

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
		void DestroyEntity(Entity entity);
		void Clear();
		bool IsEmpty() const { return m_entity_map.empty(); }
		bool SetPrimaryCamera(UUID uuid);

		Entity FindEntityByName(std::string_view name);
		Entity GetEntityByUUID(UUID uuid);

		const std::string& GetSceneName() const { return m_scene_name; }
		void SetSceneName(const std::string& name) { m_scene_name = name; }
		Camera* GetCamera();
	private:
		template<typename T>
		void OnComponentAdded(T& component)
		{
			static_assert(sizeof(T) == 0);
		}

		template<typename T>
		void OnComponentRemoved(T& component)
		{
			static_assert(sizeof(T) == 0);
		}
		
		std::string m_scene_name;
		entt::registry m_registry;
		std::unordered_map<UUID, entt::entity> m_entity_map;
		UUID m_primary_camera{ 0 };

		RenderSystem m_render_system;
		PhysicsSystem m_physics_system;

		bool m_is_running{ false };
		bool m_is_paused{ false };

		friend class Entity;
		friend class SceneSerializer;
	};

	// Explicit specializations must be at namespace scope for GCC/Clang; they are inline to avoid ODR violations.
	#include "SceneComponents.inl"
}