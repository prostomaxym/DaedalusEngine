#include "dlpch.h"

#include "Geometry.h"

using namespace Daedalus;

namespace 
{
    bool CheckIfSphereInPlane(const glm::vec4& plane, const BoundingSphere& sphere)
    {
        return glm::dot(plane, glm::vec4(sphere.position, 1.f)) > -sphere.radius;
    }
}

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
