#pragma once

#include "Serialization/SceneRecord.h"
#include "Physics/Physics.h"

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
			: handle(std::exchange(other.handle, 0)), description(std::move(other.description)) {}

		PhysicsBodyComponent& operator=(PhysicsBodyComponent&& other) noexcept
		{
			if (this != &other)
			{
				if (handle) Physics::RemoveRigidBody(handle);
				handle = std::exchange(other.handle, 0);
				description = std::move(other.description);
			}
			return *this;
		}

		~PhysicsBodyComponent()
		{
			if (handle) Physics::RemoveRigidBody(handle);
		}

	protected:
		PhysicsBodyComponent(const BodyRecord& record, bool kinematic) : description(record)
		{
			const glm::quat orientation(glm::radians(record.rotation));
			if (record.shape == "box")
				handle = Physics::CreateBoxRigidBody(record.mass, record.half_extents, record.position, orientation, kinematic);
			else if (record.shape == "sphere")
				handle = Physics::CreateSphereRigidBody(record.mass, record.radius, record.position, orientation, kinematic);
		}

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
	};
}
