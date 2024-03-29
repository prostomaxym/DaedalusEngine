#pragma once

#include "UUID.h"
#include "Systems/RenderSystem.h"
#include "Daedalus/Utils/DeltaTime.h"

#include <entt/entt.hpp>

namespace Daedalus
{
	class Entity;

	class DAEDALUS_API Scene
	{
	public:
		Scene();
		~Scene() = default;

		void OnRuntimeStart();
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

	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);
		void PrepareScene();

		entt::registry m_registry;
		std::unordered_map<UUID, entt::entity> m_entity_map;

		RenderSystem m_render_system;

		bool m_is_running{ false };
		bool m_is_paused{ false };

		friend class Entity;
	};

	// MSVC and GCC for some reason have different linking for this part
	// TODO: investigate and fix it properly
	#ifdef DL_PLATFORM_WINDOWS
		template<typename T>
		inline void Scene::OnComponentAdded(Entity entity, T& component) {}
	#endif
}
