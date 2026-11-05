#include "dlpch.h"
#include "PhysicsWorldInternal.h"
#include <algorithm>
#include <cmath>

using namespace Daedalus;
using namespace Daedalus::PhysicsDetail;

bool PhysicsWorld::ValidateCollider(const ColliderDescription& collider)
{
	if (!Finite(collider.half_extents) || !std::isfinite(collider.radius) || !std::isfinite(collider.height) ||
		collider.radius <= 0.f || collider.height <= 0.f ||
		collider.vertices.size() > 65536 || collider.indices.size() > 196608) return false;
	if (collider.shape == "box" || collider.shape == "cylinder")
		return collider.half_extents.x > 0.f && collider.half_extents.y > 0.f && collider.half_extents.z > 0.f &&
			(collider.shape != "cylinder" || collider.half_extents.x == collider.half_extents.z);
	if (collider.shape == "sphere" || collider.shape == "capsule" || collider.shape == "cone") return true;
	if (collider.shape != "convex_hull" && collider.shape != "triangle_mesh") return false;
	for (const auto& vertex : collider.vertices) if (!Finite(vertex)) return false;
	if (collider.shape == "triangle_mesh")
	{
		if (collider.vertices.size() < 3 || collider.indices.empty() || collider.indices.size() % 3 != 0) return false;
		for (const auto index : collider.indices) if (index >= collider.vertices.size()) return false;
		for (size_t i = 0; i < collider.indices.size(); i += 3)
		{
			const auto normal = glm::cross(collider.vertices[collider.indices[i + 1]] - collider.vertices[collider.indices[i]],
				collider.vertices[collider.indices[i + 2]] - collider.vertices[collider.indices[i]]);
			if (!Finite(normal) || glm::dot(normal, normal) < 1.e-12f) return false;
		}
		return true;
	}
	if (collider.vertices.size() < 4) return false;
	const auto origin = collider.vertices.front();
	glm::vec3 edge(0.f), normal(0.f);
	for (const auto& vertex : collider.vertices)
		if (glm::length(vertex - origin) > 1.e-5f) { edge = vertex - origin; break; }
	for (const auto& vertex : collider.vertices)
	{
		const auto candidate = glm::cross(edge, vertex - origin);
		if (glm::length(candidate) > 1.e-5f) { normal = glm::normalize(candidate); break; }
	}
	for (const auto& vertex : collider.vertices)
		if (std::abs(glm::dot(normal, vertex - origin)) > 1.e-5f) return true;
	return false;
}

bool PhysicsWorld::ValidateBodySettings(const PhysicsBodySettings& settings)
{
	return settings.group > 0 && settings.group <= 65535 && settings.mask <= 65535 &&
		std::isfinite(settings.friction) && settings.friction >= 0.f &&
		std::isfinite(settings.restitution) && settings.restitution >= 0.f && settings.restitution <= 1.f &&
		std::isfinite(settings.rolling_friction) && settings.rolling_friction >= 0.f &&
		std::isfinite(settings.linear_damping) && settings.linear_damping >= 0.f && settings.linear_damping <= 1.f &&
		std::isfinite(settings.angular_damping) && settings.angular_damping >= 0.f && settings.angular_damping <= 1.f;
}

std::unique_ptr<btCollisionShape> Daedalus::PhysicsDetail::Shape(const ColliderDescription& collider, std::unique_ptr<btTriangleMesh>& mesh)
{
	if (collider.shape == "box") return std::make_unique<btBoxShape>(ToBullet(collider.half_extents));
	if (collider.shape == "sphere") return std::make_unique<btSphereShape>(collider.radius);
	if (collider.shape == "capsule") return std::make_unique<btCapsuleShape>(collider.radius, collider.height);
	if (collider.shape == "cylinder") return std::make_unique<btCylinderShape>(ToBullet(collider.half_extents));
	if (collider.shape == "cone") return std::make_unique<btConeShape>(collider.radius, collider.height);
	if (collider.shape == "convex_hull")
	{
		auto hull = std::make_unique<btConvexHullShape>();
		for (const auto& vertex : collider.vertices) hull->addPoint(ToBullet(vertex), false);
		hull->recalcLocalAabb();
		return hull;
	}
	mesh = std::make_unique<btTriangleMesh>();
	for (size_t i = 0; i < collider.indices.size(); i += 3)
		mesh->addTriangle(ToBullet(collider.vertices[collider.indices[i]]), ToBullet(collider.vertices[collider.indices[i + 1]]),
			ToBullet(collider.vertices[collider.indices[i + 2]]));
	return std::make_unique<btBvhTriangleMeshShape>(mesh.get(), true);
}

bool PhysicsWorld::HasRigidBody(RigidBodyHandle handle) const { return m_impl->Find(handle) != nullptr; }

bool PhysicsWorld::SetLinearVelocity(RigidBodyHandle handle, const glm::vec3& velocity)
{
	auto* body = m_impl->Find(handle);
	if (!body || body->isStaticOrKinematicObject() || !Finite(velocity)) return false;
	body->setLinearVelocity(ToBullet(velocity));
	body->activate();
	return true;
}

bool PhysicsWorld::GetLinearVelocity(RigidBodyHandle handle, glm::vec3& velocity) const
{
	const auto* body = m_impl->Find(handle);
	if (!body) return false;
	velocity = FromBullet(body->getLinearVelocity());
	return true;
}

bool PhysicsWorld::ApplyImpulse(RigidBodyHandle handle, const glm::vec3& impulse)
{
	auto* body = m_impl->Find(handle);
	if (!body || body->isStaticOrKinematicObject() || !Finite(impulse)) return false;
	body->applyCentralImpulse(ToBullet(impulse));
	body->activate();
	return true;
}
