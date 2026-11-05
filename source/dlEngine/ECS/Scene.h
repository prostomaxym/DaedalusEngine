#pragma once

#include "UUID.h"
#include "Macros.h"
#include "Utils/DeltaTime.h"
#include "Serialization/SceneRecord.h"
#include <entt/entity/registry.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Daedalus
{
	class Entity;
	class SceneSerializer;
	class RenderSystem;
	class PhysicsSystem;
	class PhysicsWorld;
	class Camera;
	struct IDComponent;
	struct TagComponent;
	struct TransformComponent;
	struct RenderableObjectComponent;
	struct SkyboxComponent;
	struct CameraComponent;
	struct RigidBodyComponent;
	struct KinematicBodyComponent;
	struct DirectionalLightComponent;
	struct PointLightComponent;
	struct SpotLightComponent;
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
		void OnFixedUpdateRuntime(DeltaTime dt);
		void OnRenderRuntime(DeltaTime dt);

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

		// Parenting and orphaning preserve local TRS; physics ancestors require unit scale.
		[[nodiscard]] bool SetParent(UUID child, UUID parent = UUID(0));
		[[nodiscard]] UUID GetParent(UUID child) const;
		[[nodiscard]] std::span<const UUID> GetChildren(UUID parent) const;
		// Refresh after editing local transforms and before reading cached world matrices.
		size_t UpdateWorldTransforms();
		[[nodiscard]] const glm::mat4* GetWorldTransform(UUID entity) const;
		[[nodiscard]] glm::quat GetWorldOrientation(UUID entity) const;
		// Uses refreshed parent caches; scale is retained, not transferred to rigid poses.
		[[nodiscard]] bool SetWorldPose(UUID entity, const glm::vec3& position, const glm::quat& orientation);
		[[nodiscard]] bool SetEntityLink(UUID source, std::string name, UUID target);
		[[nodiscard]] UUID GetEntityLink(UUID source, std::string_view name) const;
		// Outermost to innermost source lineage; empty for non-prefab entities.
		[[nodiscard]] std::span<const PrefabOrigin> GetPrefabOrigins(UUID entity) const;
		// Explicit physics-only stepping also supports headless tools and tests.
		void StepPhysics(DeltaTime dt);

		const std::string& GetSceneName() const { return m_scene_name; }
		void SetSceneName(const std::string& name) { m_scene_name = name; }
		Camera* GetCamera();
		PhysicsWorld& GetPhysicsWorld();
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
		
		struct TransformNode
		{
			UUID parent{ 0 };
			std::vector<UUID> children;
			glm::vec3 translation{ 0.f };
			glm::vec3 rotation{ 0.f };
			glm::vec3 scale{ 1.f };
			glm::mat4 local{ 1.f };
			glm::mat4 world{ 1.f };
			glm::quat world_orientation{ 1.f, 0.f, 0.f, 0.f };
			std::map<std::string, UUID, std::less<>> links;
			bool dirty{ true };
			bool changed{ false };
		};

		void RemoveFromHierarchy(UUID entity);
		bool HasUnitScaleAncestors(UUID entity) const;
		bool UpdateWorldTransform(UUID entity);
		std::unordered_map<UUID, TransformNode> m_transform_nodes;
		std::vector<UUID> m_transform_order;
		bool m_hierarchy_dirty{ true };

		std::string m_scene_name;
		entt::registry m_registry;
		std::unordered_map<UUID, entt::entity> m_entity_map;
		std::unordered_map<UUID, std::vector<PrefabOrigin>> m_prefab_origins;
		UUID m_primary_camera{ 0 };

		std::unique_ptr<RenderSystem> m_render_system;
		std::unique_ptr<PhysicsSystem> m_physics_system;

		bool m_is_running{ false };
		bool m_is_paused{ false };
		bool m_is_iterating{ false };
		bool m_is_constructing{ false };
		uint32_t m_viewport_width{ 0 };
		uint32_t m_viewport_height{ 0 };

		friend class Entity;
		friend class SceneSerializer;
		friend class PhysicsSystem;
	};

	// Declare specializations before client Entity templates instantiate the component hooks.
	#include "SceneComponents.inl"
}