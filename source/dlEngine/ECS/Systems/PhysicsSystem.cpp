#include "dlpch.h"
#include "PhysicsSystem.h"
#include "Physics/PhysicsWorld.h"

#include "ECS/Components.h"
#include "ECS/Entity.h"
#include "ECS/Scene.h"
#include "Physics/Physics.h"
#include "Debug/Profiler.h"
#include "Debug/Log.h"

using namespace Daedalus;

PhysicsSystem::PhysicsSystem(entt::registry& reg, Scene* scene)
	: m_registry(reg), m_scene(scene), m_world(Physics::CreateWorld()) {}

PhysicsSystem::~PhysicsSystem() = default;
PhysicsSystem::PhysicsSystem(PhysicsSystem&& other) noexcept = default;

void PhysicsSystem::OnStartRuntime() const
{
	PrepareBodies();
}

void PhysicsSystem::PrepareBodies() const
{
	m_scene->UpdateWorldTransforms();
	auto prepare = [&](auto& body, entt::entity handle, bool kinematic)
	{
		if (!body.handle) body.Attach(*m_world);
		if (!body.handle) return;
		const auto id = m_registry.get<IDComponent>(handle).ID;
		if (!m_scene->HasUnitScaleAncestors(id))
		{
			glm::vec3 position;
			glm::quat orientation;
			if (m_world->GetRigidBodyTransform(body.handle, position, orientation))
			{
				Log::Write(Log::Levels::Warn, Log::Categories::ECS, "Detaching physics body from scaled ancestors");
				(void)m_scene->SetParent(id);
				(void)m_scene->SetWorldPose(id, position, orientation);
				m_scene->UpdateWorldTransforms();
			}
		}
		const bool parented = static_cast<uint64_t>(m_scene->GetParent(id)) != 0;
		if (kinematic || (parented && !body.pose_initialized) ||
			(m_world->IsStaticBody(body.handle) && (parented || body.synchronized_parent != 0)))
		{
			const auto* transform = m_scene->GetWorldTransform(id);
			m_world->SetRigidBodyTransform(body.handle, glm::vec3((*transform)[3]), m_scene->GetWorldOrientation(id));
		}
		body.pose_initialized = true;
		body.synchronized_parent = static_cast<uint64_t>(m_scene->GetParent(id));
	};
	for (auto handle : m_registry.view<RigidBodyComponent>())
		prepare(m_registry.get<RigidBodyComponent>(handle), handle, false);
	for (auto handle : m_registry.view<KinematicBodyComponent>())
		prepare(m_registry.get<KinematicBodyComponent>(handle), handle, true);
}

void PhysicsSystem::OnStopRuntime() const
{
	m_world->RegisterCollisionCallback({});
	for (auto e : m_registry.view<RigidBodyComponent>()) m_registry.get<RigidBodyComponent>(e).Release();
	for (auto e : m_registry.view<KinematicBodyComponent>()) m_registry.get<KinematicBodyComponent>(e).Release();
}

void PhysicsSystem::OnUpdateRuntime(DeltaTime dt) const
{
	DL_PROFILE_SCOPE("PhysicsSystem::Step");
	PrepareBodies();

	m_world->StepSimulation(dt.GetSeconds());
	// Collision callbacks may create, destroy or reparent entities.
	m_scene->UpdateWorldTransforms();

	DL_PROFILE_SCOPE("PhysicsSystem::SyncTransforms");
	// Refresh each ancestor before converting a descendant's world pose back to local TRS.
	for (const auto id : m_scene->m_transform_order)
	{
		auto* body = m_registry.try_get<RigidBodyComponent>(m_scene->m_entity_map.at(id));
		glm::vec3 position;
		glm::quat orientation;
		if (body && body->handle && !m_world->IsStaticBody(body->handle)
			&& m_world->GetRigidBodyTransform(body->handle, position, orientation))
		{
			(void)m_scene->SetWorldPose(id, position, orientation);
		}
		m_scene->UpdateWorldTransform(id);
		const auto* kinematic = m_registry.try_get<KinematicBodyComponent>(m_scene->m_entity_map.at(id));
		const auto* driven = kinematic ? static_cast<const PhysicsBodyComponent*>(kinematic)
			: body && m_world->IsStaticBody(body->handle) ? static_cast<const PhysicsBodyComponent*>(body) : nullptr;
		if (driven && driven->handle && (kinematic || static_cast<uint64_t>(m_scene->GetParent(id)) != 0 || driven->synchronized_parent != 0))
			m_world->SetRigidBodyTransform(driven->handle, glm::vec3((*m_scene->GetWorldTransform(id))[3]), m_scene->GetWorldOrientation(id));
	}
}