#include "dlpch.h"

#include "Geometry.h"

using namespace Daedalus;

Frustum Frustum::CalculateFrustum(const glm::mat4& pv)
{
    Frustum frust;

    frust.m_planes[Planes::Left] = glm::vec4(pv[0][3] + pv[0][0], pv[1][3] + pv[1][0], pv[2][3] + pv[2][0], pv[3][3] + pv[3][0]);
    frust.m_planes[Planes::Right] = glm::vec4(pv[0][3] - pv[0][0], pv[1][3] - pv[1][0], pv[2][3] - pv[2][0], pv[3][3] - pv[3][0]);
    frust.m_planes[Planes::Bottom] = glm::vec4(pv[0][3] + pv[0][1], pv[1][3] + pv[1][1], pv[2][3] + pv[2][1], pv[3][3] + pv[3][1]);
    frust.m_planes[Planes::Top] = glm::vec4(pv[0][3] - pv[0][1], pv[1][3] - pv[1][1], pv[2][3] - pv[2][1], pv[3][3] - pv[3][1]);
    frust.m_planes[Planes::Near] = glm::vec4(pv[0][3] + pv[0][2], pv[1][3] + pv[1][2], pv[2][3] + pv[2][2], pv[3][3] + pv[3][2]);
    frust.m_planes[Planes::Far] = glm::vec4(pv[0][3] - pv[0][2], pv[1][3] - pv[1][2], pv[2][3] - pv[2][2], pv[3][3] - pv[3][2]);


    for (auto& plane : frust.m_planes)
    {
        float length = glm::length(glm::vec3(plane));
        plane /= length;
    }

    return frust;
}

bool Frustum::SphereInFrustum(const BoundingSphere& sphere, const glm::mat4& transform) const
{
    BoundingSphere transformed_sphere = sphere.GetTransformedSphere(transform);
;
	for (const auto& plane : m_planes)
	{
		float distance = glm::dot(glm::vec3(plane), transformed_sphere.position) + plane.w;

		if (distance < -transformed_sphere.radius)
			return false;
	}

	return true;
}

BoundingSphere::BoundingSphere(const glm::vec3& pos, float rad) :
    position(pos),
    radius(rad)
{
}

BoundingSphere BoundingSphere::GetTransformedSphere(const glm::mat4& matrix) const
{
	glm::vec3 new_center = glm::vec3(matrix * glm::vec4(position, 1.0f));

	glm::vec3 scale;
	scale.x = glm::length(glm::vec3(matrix[0]));
	scale.y = glm::length(glm::vec3(matrix[1]));
	scale.z = glm::length(glm::vec3(matrix[2]));

	float max_scale = std::max(std::max(scale.x, scale.y), scale.z);

	return BoundingSphere(new_center, radius * max_scale);
}

BoundingSphere BoundingSphere::CalculateCommonBoundingSphere(const std::vector<BoundingSphere>& spheres)
{
	if (spheres.empty())
		return BoundingSphere();

	// Compute the centroid of the bounding sphere centers
	glm::vec3 centroid(0.f);
	for (const auto& sphere : spheres)
	{
		centroid += sphere.position;
	}
	centroid /= static_cast<float>(spheres.size());

	// Compute the radius
	float max_distance = 0.f;
	for (const auto& sphere : spheres)
	{
		float distance = glm::distance(centroid, sphere.position) + sphere.radius;
		if (distance > max_distance)
		{
			max_distance = distance;
		}
	}

	return BoundingSphere(centroid, max_distance);
}

AABB AABB::CalculateCommonBoundingAABB(const std::vector<AABB>& aabbs)
{
	if (aabbs.empty())
		return AABB(glm::vec3(0.f), glm::vec3(0.f));

	glm::vec3 min(std::numeric_limits<float>::max());
	glm::vec3 max(std::numeric_limits<float>::lowest());

	for (const auto& aabb : aabbs)
	{
		glm::vec3 aabbMin = aabb.GetMin();
		glm::vec3 aabbMax = aabb.GetMax();

		min = glm::min(min, aabbMin);
		max = glm::max(max, aabbMax);
	}

	return AABB(min, max);
}