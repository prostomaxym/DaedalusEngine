#include "dlpch.h"
#include "PhysicsWorld.h"
#include "PhysicsWorldInternal.h"
#include "Debug/Profiler.h"
#include <btBulletDynamicsCommon.h>
#include <atomic>
#include <cmath>
#include <thread>
#include <unordered_map>

using namespace Daedalus;
using namespace Daedalus::PhysicsDetail;

namespace Daedalus::PhysicsDetail
{
	uint64_t NextHandle()
	{
		static std::atomic<uint64_t> next_handle{ 1 };
		const auto handle = next_handle.fetch_add(1, std::memory_order_relaxed);
		if (!handle) std::abort();
		return handle;
	}

	bool Finite(const glm::vec3& value)
	{
		return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
	}

	bool ValidPose(const glm::vec3& position, const glm::quat& orientation)
	{
		return Finite(position) && std::isfinite(orientation.x) && std::isfinite(orientation.y) &&
			std::isfinite(orientation.z) && std::isfinite(orientation.w) &&
			std::isfinite(glm::dot(orientation, orientation)) && glm::dot(orientation, orientation) > 0.f;
	}
}

PhysicsWorld::Impl::Impl()
{
	m_broadphase.getOverlappingPairCache()->setInternalGhostPairCallback(&m_ghost_callback);
}

void PhysicsWorld::Impl::CheckThread() const
{
	DL_ASSERT(m_owner == std::this_thread::get_id(), Log::Categories::Physics, "PhysicsWorld must be used on its owning thread");
	if (m_owner != std::this_thread::get_id()) std::abort();
}

PhysicsWorld::Impl::~Impl()
{
	CheckThread();
	while (!m_joints.empty()) RemoveJoint(m_joints.begin()->first);
	while (!m_characters.empty()) RemoveCharacter(m_characters.begin()->first);
	for (auto& entry : m_bodies) m_world.removeRigidBody(entry.second.m_body.get());
}

btRigidBody* PhysicsWorld::Impl::Find(RigidBodyHandle handle) const
{
	CheckThread();
	const auto it = m_bodies.find(handle);
	return it == m_bodies.end() ? nullptr : it->second.m_body.get();
}

RigidBodyHandle PhysicsWorld::CreateRigidBody(const ColliderDescription& collider, float mass, const glm::vec3& position,
	const glm::quat& orientation, bool kinematic, const PhysicsBodySettings& settings)
{
	m_impl->CheckThread();
	if (!std::isfinite(mass) || mass < 0.f || !ValidPose(position, orientation) ||
		!ValidateCollider(collider) || !ValidateBodySettings(settings) ||
		(collider.shape == "triangle_mesh" && mass > 0.f && !kinematic)) return 0;
	const auto transform = Pose(position, orientation);
	Impl::Body owned;
	owned.m_shape = Shape(collider, owned.m_mesh);
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
	if (settings.trigger) owned.m_body->setCollisionFlags(owned.m_body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
	owned.m_body->setFriction(settings.friction);
	owned.m_body->setRestitution(settings.restitution);
	owned.m_body->setRollingFriction(settings.rolling_friction);
	owned.m_body->setDamping(settings.linear_damping, settings.angular_damping);
	const auto handle = NextHandle();
	auto* body = owned.m_body.get();
	m_impl->m_bodies.emplace(handle, std::move(owned));
	m_impl->m_handles.emplace(body, handle);
	m_impl->m_world.addRigidBody(body, static_cast<int>(settings.group), static_cast<int>(settings.mask));
	return handle;
}

void PhysicsWorld::Impl::Remove(RigidBodyHandle handle)
{
	CheckThread();
	const auto it = m_bodies.find(handle);
	if (it == m_bodies.end()) return;
	for (auto joint = m_joints.begin(); joint != m_joints.end();)
	{
		const auto current = joint++;
		if (current->second.m_first == handle || current->second.m_second == handle) RemoveJoint(current->first);
	}
	m_world.removeRigidBody(it->second.m_body.get());
	m_handles.erase(it->second.m_body.get());
	m_bodies.erase(it);
}

PhysicsWorld::PhysicsWorld(const glm::vec3& gravity) : m_impl(std::make_shared<Impl>())
{
	SetGravity(gravity);
}

PhysicsWorld::~PhysicsWorld() = default;

RigidBodyHandle PhysicsWorld::CreateBoxRigidBody(float mass, const glm::vec3& half_extents, const glm::vec3& position, const glm::quat& orientation, bool kinematic)
{
	ColliderDescription collider;
	collider.half_extents = half_extents;
	return CreateRigidBody(collider, mass, position, orientation, kinematic);
}

RigidBodyHandle PhysicsWorld::CreateSphereRigidBody(float mass, float radius, const glm::vec3& position, const glm::quat& orientation, bool kinematic)
{
	ColliderDescription collider;
	collider.shape = "sphere";
	collider.radius = radius;
	return CreateRigidBody(collider, mass, position, orientation, kinematic);
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

glm::vec3 PhysicsWorld::GetGravity() const
{
	m_impl->CheckThread();
	return FromBullet(m_impl->m_world.getGravity());
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
	DL_PROFILE_SCOPE("PhysicsWorld::StepSimulation");
	m_impl->CheckThread();
	if (!std::isfinite(fixed_delta) || fixed_delta <= 0.f || m_impl->m_stepping) return;
	m_impl->m_stepping = true;
	for (auto& [handle, character] : m_impl->m_characters)
		character.m_controller->setWalkDirection(ToBullet(character.m_velocity * fixed_delta));
	m_impl->m_world.stepSimulation(fixed_delta, 0);
	for (auto& [handle, character] : m_impl->m_characters) character.m_has_stepped = true;
	m_impl->CollectContacts();
	const auto callback = m_impl->m_callback;
	const auto contact_callback = m_impl->m_contact_callback;
	// Events are immutable snapshots. No Bullet pointers survive into client callbacks.
	for (const auto& contact : m_impl->m_events)
	{
		if (contact_callback) (*contact_callback)(contact);
		if (callback && !contact.trigger && contact.phase != ContactPhase::Exit &&
			m_impl->Find(contact.first) && m_impl->Find(contact.second)) (*callback)(contact.first, contact.second);
	}
	m_impl->m_stepping = false;
}
