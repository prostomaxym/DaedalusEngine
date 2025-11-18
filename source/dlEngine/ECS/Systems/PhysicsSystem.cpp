#include "dlpch.h"
#include "PhysicsSystem.h"

#include "ECS/Components.h"
#include "ECS/Entity.h"
#include "ECS/Scene.h"
#include "Physics/Physics.h"

using namespace Daedalus;

void PhysicsSystem::OnStartRuntime() const
{
	const auto rigid_body_view = m_registry.view<RigidBodyComponent>();

	// Initialize physics bodies in the scene
}

void PhysicsSystem::OnUpdateRuntime(DeltaTime dt) const
{
	Physics::StepSimulation(dt.GetSeconds());

	// Update transforms from physics for entities that have both a rigid body and a transform.
	const auto view = m_registry.view<RigidBodyComponent>();

	for (const auto e : view)
	{
		Entity entity = { e, m_scene };
		auto& rb = entity.GetComponent<RigidBodyComponent>();
		auto& tf = entity.GetComponent<TransformComponent>();

		if (rb.handle == 0)
			continue;

		glm::vec3 phys_pos;
		glm::quat phys_orient;
		if (Physics::GetRigidBodyTransform(rb.handle, phys_pos, phys_orient)
			&& !Physics::IsStaticBody(rb.handle))
		{
			// Physics returns orientation as a quaternion. TransformComponent stores rotation as
			// Euler angles in degrees (pitch, yaw, roll), so convert.
			tf.translation = phys_pos;
			tf.SetOrientationQuat(phys_orient);
		}
	}
}