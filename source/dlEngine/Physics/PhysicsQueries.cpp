#include "dlpch.h"
#include "PhysicsWorldInternal.h"
#include <algorithm>

using namespace Daedalus;
using namespace Daedalus::PhysicsDetail;

namespace
{
	bool ValidFilter(const PhysicsQueryFilter& filter)
	{
		return filter.group > 0 && filter.group <= 65535 && filter.mask <= 65535;
	}

	template<typename Base>
	struct Filtered : Base
	{
		const std::unordered_map<const btCollisionObject*, RigidBodyHandle>& m_handles;
		const PhysicsQueryFilter& m_filter;

		template<typename... Args>
		Filtered(const std::unordered_map<const btCollisionObject*, RigidBodyHandle>& handles,
			const PhysicsQueryFilter& filter, Args&&... args)
			: Base(std::forward<Args>(args)...), m_handles(handles), m_filter(filter)
		{
			this->m_collisionFilterGroup = static_cast<int>(filter.group);
			this->m_collisionFilterMask = static_cast<int>(filter.mask);
		}

		bool needsCollision(btBroadphaseProxy* proxy) const override
		{
			if (!Base::needsCollision(proxy)) return false;
			// Bullet owns the proxy; its client object is non-owning for this query only.
			const auto* object = static_cast<const btCollisionObject*>(proxy->m_clientObject);
			const auto found = m_handles.find(object);
			return found != m_handles.end() && found->second != m_filter.ignore &&
				(m_filter.include_triggers || object->hasContactResponse());
		}
	};

	struct OverlapCallback : Filtered<btCollisionWorld::ContactResultCallback>
	{
		std::vector<RigidBodyHandle> m_results;
		using Filtered::Filtered;

		btScalar addSingleResult(btManifoldPoint& point, const btCollisionObjectWrapper* first, int, int,
			const btCollisionObjectWrapper* second, int, int) override
		{
			if (point.getDistance() > 0.f) return 0.f;
			for (const auto* object : { first->getCollisionObject(), second->getCollisionObject() })
			{
				const auto found = m_handles.find(object);
				if (found != m_handles.end()) m_results.push_back(found->second);
			}
			return 0.f;
		}
	};
}

std::optional<PhysicsQueryHit> PhysicsWorld::Raycast(const glm::vec3& start, const glm::vec3& end, const PhysicsQueryFilter& filter) const
{
	m_impl->CheckThread();
	if (!Finite(start) || !Finite(end) || start == end || !ValidFilter(filter)) return std::nullopt;
	Filtered<btCollisionWorld::ClosestRayResultCallback> result(m_impl->m_handles, filter, ToBullet(start), ToBullet(end));
	m_impl->m_world.rayTest(ToBullet(start), ToBullet(end), result);
	if (!result.hasHit()) return std::nullopt;
	return PhysicsQueryHit{ m_impl->m_handles.at(result.m_collisionObject), FromBullet(result.m_hitPointWorld),
		FromBullet(result.m_hitNormalWorld), result.m_closestHitFraction };
}

std::optional<PhysicsQueryHit> PhysicsWorld::Sweep(const ColliderDescription& collider, const glm::vec3& start, const glm::vec3& end,
	const glm::quat& orientation, const PhysicsQueryFilter& filter) const
{
	m_impl->CheckThread();
	if (!ValidateCollider(collider) || collider.shape == "triangle_mesh" || !ValidPose(start, orientation) ||
		!Finite(end) || start == end || !ValidFilter(filter)) return std::nullopt;
	std::unique_ptr<btTriangleMesh> mesh;
	auto shape = Shape(collider, mesh);
	Filtered<btCollisionWorld::ClosestConvexResultCallback> result(m_impl->m_handles, filter, ToBullet(start), ToBullet(end));
	m_impl->m_world.convexSweepTest(static_cast<btConvexShape*>(shape.get()), Pose(start, orientation), Pose(end, orientation), result);
	if (!result.hasHit()) return std::nullopt;
	return PhysicsQueryHit{ m_impl->m_handles.at(result.m_hitCollisionObject), FromBullet(result.m_hitPointWorld),
		FromBullet(result.m_hitNormalWorld), result.m_closestHitFraction };
}

std::vector<RigidBodyHandle> PhysicsWorld::Overlap(const ColliderDescription& collider, const glm::vec3& position,
	const glm::quat& orientation, const PhysicsQueryFilter& filter) const
{
	m_impl->CheckThread();
	if (!ValidateCollider(collider) || collider.shape == "triangle_mesh" || !ValidPose(position, orientation) || !ValidFilter(filter)) return {};
	std::unique_ptr<btTriangleMesh> mesh;
	auto shape = Shape(collider, mesh);
	btCollisionObject probe;
	probe.setCollisionShape(shape.get());
	probe.setWorldTransform(Pose(position, orientation));
	OverlapCallback result(m_impl->m_handles, filter);
	m_impl->m_world.contactTest(&probe, result);
	std::sort(result.m_results.begin(), result.m_results.end());
	result.m_results.erase(std::unique(result.m_results.begin(), result.m_results.end()), result.m_results.end());
	return result.m_results;
}
