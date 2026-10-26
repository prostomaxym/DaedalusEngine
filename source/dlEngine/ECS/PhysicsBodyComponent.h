#pragma once

#include "Serialization/SceneRecord.h"
#include "Physics/PhysicsWorld.h"

#include <glm/gtc/quaternion.hpp>
#include <utility>

namespace Daedalus
{
	struct PhysicsBodyComponent
	{
		RigidBodyHandle handle = 0;
		BodyRecord description;

		PhysicsBodyComponent() = default;
		PhysicsBodyComponent(const PhysicsBodyComponent&) = delete;
		PhysicsBodyComponent& operator=(const PhysicsBodyComponent&) = delete;

		PhysicsBodyComponent(PhysicsBodyComponent&& other) noexcept
			: handle(std::exchange(other.handle, 0)), description(std::move(other.description)),
			m_collider(std::move(other.m_collider)), m_kinematic(other.m_kinematic), m_cleanup(std::move(other.m_cleanup))
		{
			other.m_cleanup = {};
		}

		PhysicsBodyComponent& operator=(PhysicsBodyComponent&& other) noexcept
		{
			if (this != &other)
			{
				Release();
				handle = std::exchange(other.handle, 0);
				description = std::move(other.description);
				m_collider = std::move(other.m_collider);
				m_kinematic = other.m_kinematic;
				m_cleanup = std::move(other.m_cleanup);
				other.m_cleanup = {};
			}
			return *this;
		}

		~PhysicsBodyComponent()
		{
			Release();
		}

		void Release()
		{
			if (m_cleanup) m_cleanup();
			m_cleanup = {};
			handle = 0;
		}

		void Attach(PhysicsWorld& world)
		{
			Release();
			const glm::quat orientation(glm::radians(m_collider.rotation));
			if (m_collider.shape == "box")
				handle = world.CreateBoxRigidBody(m_collider.mass, m_collider.half_extents, m_collider.position, orientation, m_kinematic);
			else if (m_collider.shape == "sphere")
				handle = world.CreateSphereRigidBody(m_collider.mass, m_collider.radius, m_collider.position, orientation, m_kinematic);
			if (handle) m_cleanup = world.MakeBodyCleanup(handle);
		}

	protected:
		PhysicsBodyComponent(const BodyRecord& record, bool kinematic)
			: description(record), m_collider(record), m_kinematic(kinematic) {}

		static BodyRecord Box(float mass, const glm::vec3& half_extents, const glm::vec3& position, const glm::quat& orientation)
		{
			BodyRecord record;
			record.mass = mass;
			record.half_extents = half_extents;
			record.position = position;
			record.rotation = glm::degrees(glm::eulerAngles(orientation));
			return record;
		}

		static BodyRecord Sphere(float mass, float radius, const glm::vec3& position, const glm::quat& orientation)
		{
			auto record = Box(mass, glm::vec3(0.5f), position, orientation);
			record.shape = "sphere";
			record.radius = radius;
			return record;
		}

	private:
		BodyRecord m_collider;
		bool m_kinematic{ false };
		std::function<void()> m_cleanup;
	};
}
