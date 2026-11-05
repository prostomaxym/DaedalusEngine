#include "dlpch.h"
#include "PhysicsWorldInternal.h"
#include <BulletDynamics/ConstraintSolver/btFixedConstraint.h>
#include <cmath>
#include <limits>

using namespace Daedalus;
using namespace Daedalus::PhysicsDetail;

bool PhysicsWorld::ValidateJointSettings(const JointSettings& settings)
{
	if (settings.type != "fixed" && settings.type != "point" && settings.type != "hinge" &&
		settings.type != "slider" && settings.type != "cone") return false;
	return Finite(settings.pivot_a) && Finite(settings.pivot_b) && Finite(settings.rotation_a) && Finite(settings.rotation_b) &&
		std::isfinite(settings.lower_limit) && std::isfinite(settings.upper_limit) && settings.lower_limit <= settings.upper_limit &&
		std::isfinite(settings.angular_lower_limit) && std::isfinite(settings.angular_upper_limit) &&
		settings.angular_lower_limit <= settings.angular_upper_limit &&
		Finite(settings.swing_limits) && settings.swing_limits.x >= 0.f && settings.swing_limits.x <= 180.f &&
		settings.swing_limits.y >= 0.f && settings.swing_limits.y <= 180.f &&
		settings.swing_limits.z >= 0.f && settings.swing_limits.z <= 180.f &&
		std::isfinite(settings.breaking_impulse) && settings.breaking_impulse >= 0.f;
}

JointHandle PhysicsWorld::CreateJoint(RigidBodyHandle first, RigidBodyHandle second, const JointSettings& settings)
{
	auto* a = m_impl->Find(first);
	auto* b = m_impl->Find(second);
	if (!a || !b || a == b || !ValidateJointSettings(settings) || (a->isStaticOrKinematicObject() && b->isStaticOrKinematicObject())) return 0;
	const auto frame_a = Pose(settings.pivot_a, glm::quat(glm::radians(settings.rotation_a)));
	const auto frame_b = Pose(settings.pivot_b, glm::quat(glm::radians(settings.rotation_b)));
	std::unique_ptr<btTypedConstraint> constraint;
	if (settings.type == "fixed") constraint = std::make_unique<btFixedConstraint>(*a, *b, frame_a, frame_b);
	else if (settings.type == "point") constraint = std::make_unique<btPoint2PointConstraint>(*a, *b, ToBullet(settings.pivot_a), ToBullet(settings.pivot_b));
	else if (settings.type == "hinge")
	{
		auto hinge = std::make_unique<btHingeConstraint>(*a, *b, frame_a, frame_b);
		hinge->setLimit(glm::radians(settings.lower_limit), glm::radians(settings.upper_limit));
		constraint = std::move(hinge);
	}
	else if (settings.type == "slider")
	{
		auto slider = std::make_unique<btSliderConstraint>(*a, *b, frame_a, frame_b, true);
		slider->setLowerLinLimit(settings.lower_limit);
		slider->setUpperLinLimit(settings.upper_limit);
		slider->setLowerAngLimit(glm::radians(settings.angular_lower_limit));
		slider->setUpperAngLimit(glm::radians(settings.angular_upper_limit));
		constraint = std::move(slider);
	}
	else
	{
		auto cone = std::make_unique<btConeTwistConstraint>(*a, *b, frame_a, frame_b);
		cone->setLimit(glm::radians(settings.swing_limits.x), glm::radians(settings.swing_limits.y), glm::radians(settings.swing_limits.z));
		constraint = std::move(cone);
	}
	if (settings.breaking_impulse > 0.f) constraint->setBreakingImpulseThreshold(settings.breaking_impulse);
	constraint->setDbgDrawSize(0.5f);
	const auto handle = NextHandle();
	m_impl->m_world.addConstraint(constraint.get(), !settings.collide_connected);
	m_impl->m_joints.emplace(handle, Impl::Joint{ first, second, std::move(constraint) });
	return handle;
}

void PhysicsWorld::Impl::RemoveJoint(JointHandle handle)
{
	CheckThread();
	const auto it = m_joints.find(handle);
	if (it == m_joints.end()) return;
	m_world.removeConstraint(it->second.m_constraint.get());
	m_joints.erase(it);
}

void PhysicsWorld::RemoveJoint(JointHandle handle) { m_impl->RemoveJoint(handle); }
bool PhysicsWorld::HasJoint(JointHandle handle) const
{
	m_impl->CheckThread();
	return m_impl->m_joints.contains(handle);
}

bool PhysicsWorld::IsJointEnabled(JointHandle handle) const
{
	m_impl->CheckThread();
	const auto it = m_impl->m_joints.find(handle);
	return it != m_impl->m_joints.end() && it->second.m_constraint->isEnabled();
}

std::function<void()> PhysicsWorld::MakeJointCleanup(JointHandle handle) const
{
	std::weak_ptr<Impl> owner = m_impl;
	return [owner, handle] { if (auto world = owner.lock()) world->RemoveJoint(handle); };
}
