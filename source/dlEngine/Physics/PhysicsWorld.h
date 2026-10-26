#pragma once

#include "Macros.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Daedalus
{
	using RigidBodyHandle = uint64_t;

	class DAEDALUS_API PhysicsWorld
	{
	public:
		using CollisionCallback = std::function<void(RigidBodyHandle, RigidBodyHandle)>;

		explicit PhysicsWorld(const glm::vec3& gravity = glm::vec3(0.f, -9.81f, 0.f));
		~PhysicsWorld();
		PhysicsWorld(const PhysicsWorld&) = delete;
		PhysicsWorld& operator=(const PhysicsWorld&) = delete;
		PhysicsWorld(PhysicsWorld&&) = delete;
		PhysicsWorld& operator=(PhysicsWorld&&) = delete;

		// One integration tick. The caller owns fixed-step accumulation.
		void StepSimulation(float fixed_delta = 1.f / 60.f);
		RigidBodyHandle CreateBoxRigidBody(float mass, const glm::vec3& half_extents, const glm::vec3& position,
			const glm::quat& orientation = glm::quat(1.f, 0.f, 0.f, 0.f), bool kinematic = false);
		RigidBodyHandle CreateSphereRigidBody(float mass, float radius, const glm::vec3& position,
			const glm::quat& orientation = glm::quat(1.f, 0.f, 0.f, 0.f), bool kinematic = false);
		void RemoveRigidBody(RigidBodyHandle handle);
		void SetGravity(const glm::vec3& gravity);
		bool GetRigidBodyTransform(RigidBodyHandle handle, glm::vec3& position, glm::quat& orientation) const;
		bool SetRigidBodyTransform(RigidBodyHandle handle, const glm::vec3& position, const glm::quat& orientation);
		bool IsStaticBody(RigidBodyHandle handle) const;
		void RegisterCollisionCallback(CollisionCallback callback);
		std::function<void()> MakeBodyCleanup(RigidBodyHandle handle) const;

	private:
		struct Impl;
		// Components hold weak references only, so cleanup cannot prolong the world's lifetime.
		std::shared_ptr<Impl> m_impl;
	};
}
