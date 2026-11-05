#pragma once

#include "PhysicsWorld.h"
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include <map>
#include <thread>
#include <unordered_map>

namespace Daedalus
{
	namespace PhysicsDetail
	{
		inline btVector3 ToBullet(const glm::vec3& value) { return { value.x, value.y, value.z }; }
		inline glm::vec3 FromBullet(const btVector3& value) { return { value.x(), value.y(), value.z() }; }
		inline btTransform Pose(const glm::vec3& position, const glm::quat& orientation)
		{
			const auto q = glm::normalize(orientation);
			return btTransform(btQuaternion(q.x, q.y, q.z, q.w), ToBullet(position));
		}
		bool Finite(const glm::vec3& value);
		bool ValidPose(const glm::vec3& position, const glm::quat& orientation);
		std::unique_ptr<btCollisionShape> Shape(const ColliderDescription& collider, std::unique_ptr<btTriangleMesh>& mesh);
		uint64_t NextHandle();
	}

	struct PhysicsWorld::Impl
	{
		struct Body
		{
			std::unique_ptr<btTriangleMesh> m_mesh;
			std::unique_ptr<btCollisionShape> m_shape;
			std::unique_ptr<btDefaultMotionState> m_motion;
			std::unique_ptr<btRigidBody> m_body;
		};
		struct Character
		{
			std::unique_ptr<btCapsuleShape> m_shape;
			std::unique_ptr<btPairCachingGhostObject> m_ghost;
			std::unique_ptr<btKinematicCharacterController> m_controller;
			glm::vec3 m_velocity{ 0.f };
			bool m_has_stepped = false;
		};
		struct Joint
		{
			RigidBodyHandle m_first;
			RigidBodyHandle m_second;
			std::unique_ptr<btTypedConstraint> m_constraint;
		};

		const std::thread::id m_owner{ std::this_thread::get_id() };
		btDefaultCollisionConfiguration m_configuration;
		btCollisionDispatcher m_dispatcher{ &m_configuration };
		btGhostPairCallback m_ghost_callback;
		btDbvtBroadphase m_broadphase;
		btSequentialImpulseConstraintSolver m_solver;
		btDiscreteDynamicsWorld m_world{ &m_dispatcher, &m_broadphase, &m_solver, &m_configuration };
		std::unordered_map<RigidBodyHandle, Body> m_bodies;
		std::unordered_map<CharacterHandle, Character> m_characters;
		std::unordered_map<JointHandle, Joint> m_joints;
		std::unordered_map<const btCollisionObject*, RigidBodyHandle> m_handles;
		std::map<std::pair<RigidBodyHandle, RigidBodyHandle>, PhysicsContact> m_previous_contacts;
		std::vector<PhysicsContact> m_events;
		std::shared_ptr<CollisionCallback> m_callback;
		std::shared_ptr<ContactCallback> m_contact_callback;
		bool m_stepping{ false };

		Impl();
		~Impl();
		void CheckThread() const;
		btRigidBody* Find(RigidBodyHandle handle) const;
		void Remove(RigidBodyHandle handle);
		void RemoveCharacter(CharacterHandle handle);
		void RemoveJoint(JointHandle handle);
		void CollectContacts();
	};
}
