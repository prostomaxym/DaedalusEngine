#pragma once

#include "Macros.h"
#include "PhysicsTypes.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Daedalus
{
	class DAEDALUS_API PhysicsWorld
	{
	public:
		using CollisionCallback = std::function<void(RigidBodyHandle, RigidBodyHandle)>;
		using ContactCallback = std::function<void(const PhysicsContact&)>;
		using DebugLineCallback = std::function<void(const glm::vec3&, const glm::vec3&, const glm::vec3&)>;
		static bool ValidateCollider(const ColliderDescription& collider);
		static bool ValidateBodySettings(const PhysicsBodySettings& settings);
		static bool ValidateCharacterSettings(const CharacterSettings& settings);
		static bool ValidateJointSettings(const JointSettings& settings);

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
		RigidBodyHandle CreateRigidBody(const ColliderDescription& collider, float mass, const glm::vec3& position,
			const glm::quat& orientation = glm::quat(1.f, 0.f, 0.f, 0.f), bool kinematic = false,
			const PhysicsBodySettings& settings = {});
		bool HasRigidBody(RigidBodyHandle handle) const;
		bool SetLinearVelocity(RigidBodyHandle handle, const glm::vec3& velocity);
		bool GetLinearVelocity(RigidBodyHandle handle, glm::vec3& velocity) const;
		bool ApplyImpulse(RigidBodyHandle handle, const glm::vec3& impulse);
		std::optional<PhysicsQueryHit> Raycast(const glm::vec3& start, const glm::vec3& end, const PhysicsQueryFilter& filter = {}) const;
		std::optional<PhysicsQueryHit> Sweep(const ColliderDescription& collider, const glm::vec3& start, const glm::vec3& end,
			const glm::quat& orientation = glm::quat(1.f, 0.f, 0.f, 0.f), const PhysicsQueryFilter& filter = {}) const;
		std::vector<RigidBodyHandle> Overlap(const ColliderDescription& collider, const glm::vec3& position,
			const glm::quat& orientation = glm::quat(1.f, 0.f, 0.f, 0.f), const PhysicsQueryFilter& filter = {}) const;
		CharacterHandle CreateCharacter(const CharacterSettings& settings, const glm::vec3& position);
		void RemoveCharacter(CharacterHandle handle);
		bool HasCharacter(CharacterHandle handle) const;
		bool SetCharacterVelocity(CharacterHandle handle, const glm::vec3& velocity);
		bool GetCharacterPosition(CharacterHandle handle, glm::vec3& position) const;
		bool WarpCharacter(CharacterHandle handle, const glm::vec3& position);
		bool IsCharacterGrounded(CharacterHandle handle) const;
		bool JumpCharacter(CharacterHandle handle);
		std::function<void()> MakeCharacterCleanup(CharacterHandle handle) const;
		JointHandle CreateJoint(RigidBodyHandle first, RigidBodyHandle second, const JointSettings& settings);
		void RemoveJoint(JointHandle handle);
		bool HasJoint(JointHandle handle) const;
		bool IsJointEnabled(JointHandle handle) const;
		std::function<void()> MakeJointCleanup(JointHandle handle) const;
		void DrawDebug(const DebugLineCallback& line) const;
		void SetGravity(const glm::vec3& gravity);
		glm::vec3 GetGravity() const;
		bool GetRigidBodyTransform(RigidBodyHandle handle, glm::vec3& position, glm::quat& orientation) const;
		bool SetRigidBodyTransform(RigidBodyHandle handle, const glm::vec3& position, const glm::quat& orientation);
		bool IsStaticBody(RigidBodyHandle handle) const;
		void RegisterCollisionCallback(CollisionCallback callback);
		void RegisterContactCallback(ContactCallback callback);
		// Valid until the next tick; removal produces an Exit on that next tick.
		std::span<const PhysicsContact> GetContactEvents() const;
		std::function<void()> MakeBodyCleanup(RigidBodyHandle handle) const;

	private:
		struct Impl;
		// Components hold weak references only, so cleanup cannot prolong the world's lifetime.
		std::shared_ptr<Impl> m_impl;
	};
}
