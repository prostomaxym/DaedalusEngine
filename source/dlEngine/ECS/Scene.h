#pragma once

#include "UUID.h"
#include "Systems/RenderSystem.h"
#include "Utils/DeltaTime.h"
#include "Components.h"

#include <entt/entt.hpp>

namespace Daedalus
{
	class Entity;
	struct NativeScriptComponent;

	class DAEDALUS_API Scene
	{
	public:
		Scene();
		~Scene() = default;

		void OnRuntimeStart(const std::string& scenename = "");
		void OnRuntimeStop();
		void OnUpdateRuntime(DeltaTime dt);

		void OnViewportResize(uint32_t width, uint32_t height);

		bool IsRunning() const { return m_is_running; }
		bool IsPaused() const { return m_is_paused; }

		void SetPaused(bool paused) { m_is_paused = paused; }

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
		void DestroyEntity(Entity entity);

		Entity FindEntityByName(std::string_view name);
		Entity GetEntityByUUID(UUID uuid);

		const std::string& GetSceneName() const { return m_scene_name; }
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
		
		#include "SceneComponents.inl" // Sorry for that, only way I figured out to make it. All other options give me linking errors because circular dependencies

		std::string m_scene_name;
		entt::registry m_registry;
		std::unordered_map<UUID, entt::entity> m_entity_map;

		RenderSystem m_render_system;

		bool m_is_running{ false };
		bool m_is_paused{ false };

		friend class Entity;
	};
}