#include "dlpch.h"

#include "CommonStructures.h"

using namespace Daedalus;

float Plane::GetSignedDistanceToPlane(const glm::vec3& point) const
{
    return glm::dot(normal, point) - distance;
}

bool BoundingSphere::IsOnOrForwardPlane(const Plane& plane) const
{
    return plane.GetSignedDistanceToPlane(position) > -radius;
}

bool Frustum::IsInFrustum(const BoundingSphere& sphere, const glm::mat4& transform) const
{
	glm::vec3 scale;
	scale.x = glm::length(glm::vec3(transform[0]));
	scale.y = glm::length(glm::vec3(transform[1]));
	scale.z = glm::length(glm::vec3(transform[2]));

	const glm::vec3 global_center(transform * glm::vec4(sphere.position, 1.f));
	const float max_scale = std::max(std::max(scale.x, scale.y), scale.z);

	BoundingSphere global_sphere = { global_center, sphere.radius * max_scale * 0.5f };

	return global_sphere.IsOnOrForwardPlane(left_face) &&
		global_sphere.IsOnOrForwardPlane(right_face) &&
		global_sphere.IsOnOrForwardPlane(far_face) &&
		global_sphere.IsOnOrForwardPlane(near_face) &&
		global_sphere.IsOnOrForwardPlane(top_face) &&
		global_sphere.IsOnOrForwardPlane(bottom_face);
}