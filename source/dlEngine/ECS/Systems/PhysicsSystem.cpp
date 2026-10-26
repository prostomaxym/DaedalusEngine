#include "dlpch.h"
#include "PhysicsSystem.h"

#include "ECS/Components.h"
#include "ECS/Entity.h"
#include "ECS/Scene.h"
#include "Physics/Physics.h"

using namespace Daedalus;

PhysicsSystem::PhysicsSystem(entt::registry& reg, Scene* scene)
	: m_registry(reg), m_scene(scene), m_world(Physics::CreateWorld()) {}

void PhysicsSystem::OnStartRuntime() const
{
	for (auto e : m_registry.view<RigidBodyComponent>())
	{
		auto& body = m_registry.get<RigidBodyComponent>(e);
		if (!body.handle) body.Attach(*m_world);
	}
	for (auto e : m_registry.view<KinematicBodyComponent>())
	{
		auto& body = m_registry.get<KinematicBodyComponent>(e);
		if (!body.handle) body.Attach(*m_world);
	}
	const auto kinematic_body_view = m_registry.view<KinematicBodyComponent>();

	for (const auto e : kinematic_body_view)
	{
		Entity entity = { e, m_scene };
		auto& kb = entity.GetComponent<KinematicBodyComponent>();
		auto& tf = entity.GetComponent<TransformComponent>();
		if (kb.handle == 0)
			continue;
		// Set initial transform from TransformComponent to physics body
		m_world->SetRigidBodyTransform(kb.handle, tf.translation, tf.GetOrientationQuat());
	}
}

void PhysicsSystem::OnStopRuntime() const
{
	m_world->RegisterCollisionCallback({});
	for (auto e : m_registry.view<RigidBodyComponent>()) m_registry.get<RigidBodyComponent>(e).Release();
	for (auto e : m_registry.view<KinematicBodyComponent>()) m_registry.get<KinematicBodyComponent>(e).Release();
}

void PhysicsSystem::OnUpdateRuntime(DeltaTime dt) const
{
	const auto kinematic_body_view = m_registry.view<KinematicBodyComponent>();

	for (const auto e : kinematic_body_view)
	{
		Entity entity = { e, m_scene };
		auto& kb = entity.GetComponent<KinematicBodyComponent>();
		auto& tf = entity.GetComponent<TransformComponent>();
		if (kb.handle == 0)
			continue;

		m_world->SetRigidBodyTransform(kb.handle, tf.translation, tf.GetOrientationQuat());
	}

	m_world->StepSimulation(dt.GetSeconds());

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
		if (m_world->GetRigidBodyTransform(rb.handle, phys_pos, phys_orient)
			&& !m_world->IsStaticBody(rb.handle))
		{
			// Physics returns orientation as a quaternion. TransformComponent stores rotation as
			// Euler angles in degrees (pitch, yaw, roll), so convert.
			tf.translation = phys_pos;
			tf.SetOrientationQuat(phys_orient);
		}
	}
}