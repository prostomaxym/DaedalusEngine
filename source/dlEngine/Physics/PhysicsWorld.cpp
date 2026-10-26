#include "dlpch.h"
#include "PhysicsWorld.h"
#include <btBulletDynamicsCommon.h>
#include <atomic>
#include <cmath>
#include <thread>
#include <unordered_map>

using namespace Daedalus;

namespace
{
	std::atomic<RigidBodyHandle> s_next_body{ 1 };

	bool Finite(const glm::vec3& value)
	{
		return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
	}

	bool ValidPose(const glm::vec3& position, const glm::quat& orientation)
	{
		return Finite(position) && std::isfinite(orientation.x) && std::isfinite(orientation.y) &&
			std::isfinite(orientation.z) && std::isfinite(orientation.w) && glm::dot(orientation, orientation) > 0.f;
	}
}

struct PhysicsWorld::Impl
{
	struct Body
	{
		std::unique_ptr<btCollisionShape> m_shape;
		std::unique_ptr<btDefaultMotionState> m_motion;
		std::unique_ptr<btRigidBody> m_body;
	};

	const std::thread::id m_owner{ std::this_thread::get_id() };
	btDefaultCollisionConfiguration m_configuration;
	btCollisionDispatcher m_dispatcher{ &m_configuration };
	btDbvtBroadphase m_broadphase;
	btSequentialImpulseConstraintSolver m_solver;
	btDiscreteDynamicsWorld m_world{ &m_dispatcher, &m_broadphase, &m_solver, &m_configuration };
	std::unordered_map<RigidBodyHandle, Body> m_bodies;
	std::unordered_map<const btCollisionObject*, RigidBodyHandle> m_handles;
	std::vector<std::pair<RigidBodyHandle, RigidBodyHandle>> m_contacts;
	std::shared_ptr<CollisionCallback> m_callback;
	bool m_stepping{ false };

	void CheckThread() const
	{
		DL_ASSERT(m_owner == std::this_thread::get_id(), Log::Categories::Physics, "PhysicsWorld must be used on its owning thread");
		if (m_owner != std::this_thread::get_id()) std::abort();
	}

	~Impl()
	{
		CheckThread();
		for (auto& entry : m_bodies) m_world.removeRigidBody(entry.second.m_body.get());
	}

	btRigidBody* Find(RigidBodyHandle handle) const
	{
		CheckThread();
		const auto it = m_bodies.find(handle);
		return it == m_bodies.end() ? nullptr : it->second.m_body.get();
	}

	RigidBodyHandle Create(std::unique_ptr<btCollisionShape> shape, float mass, const glm::vec3& position,
		const glm::quat& orientation, bool kinematic)
	{
		CheckThread();
		if (!std::isfinite(mass) || mass < 0.f || !ValidPose(position, orientation)) return 0;
		const auto rotation = glm::normalize(orientation);
		const btTransform transform(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w), btVector3(position.x, position.y, position.z));
		Body owned;
		owned.m_shape = std::move(shape);
		owned.m_motion = std::make_unique<btDefaultMotionState>(transform);
		const float body_mass = kinematic ? 0.f : mass;
		btVector3 inertia(0, 0, 0);
		if (body_mass > 0.f) owned.m_shape->calculateLocalInertia(body_mass, inertia);
		btRigidBody::btRigidBodyConstructionInfo info(body_mass, owned.m_motion.get(), owned.m_shape.get(), inertia);
		owned.m_body = std::make_unique<btRigidBody>(info);
		if (kinematic)
		{
			owned.m_body->setCollisionFlags(owned.m_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			owned.m_body->setActivationState(DISABLE_DEACTIVATION);
		}
		const auto handle = s_next_body.fetch_add(1, std::memory_order_relaxed);
		if (!handle) std::abort();
		auto* body = owned.m_body.get();
		m_bodies.emplace(handle, std::move(owned));
		m_handles.emplace(body, handle);
		m_world.addRigidBody(body);
		return handle;
	}

	void Remove(RigidBodyHandle handle)
	{
		CheckThread();
		const auto it = m_bodies.find(handle);
		if (it == m_bodies.end()) return;
		m_world.removeRigidBody(it->second.m_body.get());
		m_handles.erase(it->second.m_body.get());
		m_bodies.erase(it);
	}
};

PhysicsWorld::PhysicsWorld(const glm::vec3& gravity) : m_impl(std::make_shared<Impl>())
{
	SetGravity(gravity);
}

PhysicsWorld::~PhysicsWorld() = default;

RigidBodyHandle PhysicsWorld::CreateBoxRigidBody(float mass, const glm::vec3& half_extents, const glm::vec3& position, const glm::quat& orientation, bool kinematic)
{
	if (!Finite(half_extents) || half_extents.x <= 0.f || half_extents.y <= 0.f || half_extents.z <= 0.f) return 0;
	return m_impl->Create(std::make_unique<btBoxShape>(btVector3(half_extents.x, half_extents.y, half_extents.z)), mass, position, orientation, kinematic);
}

RigidBodyHandle PhysicsWorld::CreateSphereRigidBody(float mass, float radius, const glm::vec3& position, const glm::quat& orientation, bool kinematic)
{
	if (!std::isfinite(radius) || radius <= 0.f) return 0;
	return m_impl->Create(std::make_unique<btSphereShape>(radius), mass, position, orientation, kinematic);
}

void PhysicsWorld::RemoveRigidBody(RigidBodyHandle handle) { m_impl->Remove(handle); }

void PhysicsWorld::SetGravity(const glm::vec3& gravity)
{
	m_impl->CheckThread();
	if (Finite(gravity)) m_impl->m_world.setGravity(btVector3(gravity.x, gravity.y, gravity.z));
}

bool PhysicsWorld::GetRigidBodyTransform(RigidBodyHandle handle, glm::vec3& position, glm::quat& orientation) const
{
	const auto* body = m_impl->Find(handle);
	if (!body) return false;
	const auto& transform = body->getWorldTransform();
	const auto& p = transform.getOrigin();
	const auto& q = transform.getRotation();
	position = glm::vec3(p.x(), p.y(), p.z());
	orientation = glm::quat(q.w(), q.x(), q.y(), q.z());
	return true;
}

bool PhysicsWorld::SetRigidBodyTransform(RigidBodyHandle handle, const glm::vec3& position, const glm::quat& orientation)
{
	auto* body = m_impl->Find(handle);
	if (!body || !ValidPose(position, orientation)) return false;
	const auto rotation = glm::normalize(orientation);
	const btTransform transform(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w), btVector3(position.x, position.y, position.z));
	body->setWorldTransform(transform);
	body->setInterpolationWorldTransform(transform);
	body->getMotionState()->setWorldTransform(transform);
	m_impl->m_world.updateSingleAabb(body);
	body->activate();
	return true;
}

bool PhysicsWorld::IsStaticBody(RigidBodyHandle handle) const
{
	const auto* body = m_impl->Find(handle);
	return body && body->isStaticObject();
}

void PhysicsWorld::RegisterCollisionCallback(CollisionCallback callback)
{
	m_impl->CheckThread();
	m_impl->m_callback = callback ? std::make_shared<CollisionCallback>(std::move(callback)) : nullptr;
}

std::function<void()> PhysicsWorld::MakeBodyCleanup(RigidBodyHandle handle) const
{
	std::weak_ptr<Impl> owner = m_impl;
	return [owner, handle] { if (auto world = owner.lock()) world->Remove(handle); };
}

void PhysicsWorld::StepSimulation(float fixed_delta)
{
	m_impl->CheckThread();
	if (!std::isfinite(fixed_delta) || fixed_delta <= 0.f || m_impl->m_stepping) return;
	m_impl->m_stepping = true;
	m_impl->m_world.stepSimulation(fixed_delta, 0);
	m_impl->m_contacts.clear();
	const auto callback = m_impl->m_callback;
	if (callback)
	{
		for (int i = 0; i < m_impl->m_dispatcher.getNumManifolds(); ++i)
		{
			const auto* manifold = m_impl->m_dispatcher.getManifoldByIndexInternal(i);
			bool touching = false;
			for (int j = 0; j < manifold->getNumContacts(); ++j)
				touching = touching || manifold->getContactPoint(j).getDistance() <= 0;
			if (!touching) continue;
			const auto a = m_impl->m_handles.find(manifold->getBody0());
			const auto b = m_impl->m_handles.find(manifold->getBody1());
			if (a == m_impl->m_handles.end() || b == m_impl->m_handles.end()) continue;
			m_impl->m_contacts.emplace_back((std::min)(a->second, b->second), (std::max)(a->second, b->second));
		}
		std::sort(m_impl->m_contacts.begin(), m_impl->m_contacts.end());
		m_impl->m_contacts.erase(std::unique(m_impl->m_contacts.begin(), m_impl->m_contacts.end()), m_impl->m_contacts.end());
		// Deliver after reading Bullet manifolds so callbacks may remove bodies safely.
		for (const auto& pair : m_impl->m_contacts)
		{
			if (m_impl->Find(pair.first) && m_impl->Find(pair.second)) (*callback)(pair.first, pair.second);
		}
	}
	m_impl->m_stepping = false;
}
