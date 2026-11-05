#pragma once

#include "Serialization/SceneRecord.h"
#include <functional>
#include <utility>

namespace Daedalus
{
	struct PhysicsResource
	{
		uint64_t handle = 0;
		PhysicsResource() = default;
		PhysicsResource(const PhysicsResource&) = delete;
		PhysicsResource& operator=(const PhysicsResource&) = delete;
		PhysicsResource(PhysicsResource&& other) noexcept
			: handle(std::exchange(other.handle, 0)), m_cleanup(std::exchange(other.m_cleanup, {})) {}
		PhysicsResource& operator=(PhysicsResource&& other) noexcept
		{
			if (this != &other)
			{
				Release();
				handle = std::exchange(other.handle, 0);
				m_cleanup = std::exchange(other.m_cleanup, {});
			}
			return *this;
		}
		~PhysicsResource() { Release(); }
		void Release()
		{
			if (m_cleanup) m_cleanup();
			m_cleanup = {};
			handle = 0;
		}
		void Own(uint64_t value, std::function<void()> cleanup)
		{
			Release();
			handle = value;
			m_cleanup = std::move(cleanup);
		}
	private:
		std::function<void()> m_cleanup;
	};

	struct CharacterControllerComponent : PhysicsResource
	{
		CharacterSettings description;
		glm::vec3 velocity{ 0.f };
		bool jump_requested = false;
		bool pose_initialized = false;

		CharacterControllerComponent() = default;
		explicit CharacterControllerComponent(const CharacterSettings& settings) : description(settings) {}
	};

	struct JointComponent
	{
		struct RuntimeJoint : PhysicsResource
		{
			RigidBodyHandle first = 0;
			RigidBodyHandle second = 0;
		};
		std::vector<JointRecord> description;
		std::vector<RuntimeJoint> joints;
		JointComponent() = default;
		JointComponent(const JointComponent&) = delete;
		JointComponent& operator=(const JointComponent&) = delete;
		JointComponent(JointComponent&&) noexcept = default;
		JointComponent& operator=(JointComponent&&) noexcept = default;
		explicit JointComponent(const std::vector<JointRecord>& records) : description(records) {}
	};
}
