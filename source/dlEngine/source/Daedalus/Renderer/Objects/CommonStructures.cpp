#include "dlpch.h"

#include "CommonStructures.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace Daedalus;

namespace {
    bool CheckIfSphereInPlane(const glm::vec4& plane, const BoundingSphere& sphere)
    {
        return glm::dot(plane, glm::vec4(sphere.position, 1.f)) > -sphere.radius;
    }
}
Frustum Frustum::CalculateFrustum(const glm::mat4& view_projection)
{
    Frustum frust;

    const glm::mat4 column_major_view_projection = glm::transpose(view_projection);
    const float* clip = glm::value_ptr(column_major_view_projection);

    glm::vec4 right;
    right[0] = clip[3] - clip[0];
    right[1] = clip[7] - clip[4];
    right[2] = clip[11] - clip[8];
    right[3] = clip[15] - clip[12];
    frust.m_right = glm::normalize(right);;

    glm::vec4 left;
    left[0] = clip[3] + clip[0];
    left[1] = clip[7] + clip[4];
    left[2] = clip[11] + clip[8];
    left[3] = clip[15] + clip[12];
    frust.m_left = glm::normalize(left);

    glm::vec4 bottom;
    bottom[0] = clip[3] + clip[1];
    bottom[1] = clip[7] + clip[5];
    bottom[2] = clip[11] + clip[9];
    bottom[3] = clip[15] + clip[13];
    frust.m_bottom = glm::normalize(bottom);

    glm::vec4 top;
    top[0] = clip[3] - clip[1];
    top[1] = clip[7] - clip[5];
    top[2] = clip[11] - clip[9];
    top[3] = clip[15] - clip[13];
    frust.m_top = glm::normalize(top);

    glm::vec4 far;
    far[0] = clip[3] - clip[2];
    far[1] = clip[7] - clip[6];
    far[2] = clip[11] - clip[10];
    far[3] = clip[15] - clip[14];
    frust.m_far = glm::normalize(far);

    glm::vec4 near;
    near[0] = clip[3] + clip[2];
    near[1] = clip[7] + clip[6];
    near[2] = clip[11] + clip[10];
    near[3] = clip[15] + clip[14];
    frust.m_near = glm::normalize(near);

    return frust;
}

bool Frustum::SphereInFrustum(const BoundingSphere& sphere, const glm::mat4& transform) const
{
    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec3 skew_unused;
    glm::vec4 perspective_unused;;
    glm::decompose(transform, scale, rotation, translation, skew_unused, perspective_unused);

    const float max_scale = std::max(std::max(std::max(scale.x, scale.y), scale.z), 0.0f);
    const float scaled_radius = sphere.radius * max_scale;
    const auto sphere_offset = (rotation * sphere.position) * max_scale;
    const glm::vec3 world_center = translation + sphere_offset;

    BoundingSphere transformed_sphere = { world_center, scaled_radius };

    return CheckIfSphereInPlane(m_near, transformed_sphere)
        && CheckIfSphereInPlane(m_far, transformed_sphere)
        && CheckIfSphereInPlane(m_left, transformed_sphere)
        && CheckIfSphereInPlane(m_right, transformed_sphere)
        && CheckIfSphereInPlane(m_top, transformed_sphere)
        && CheckIfSphereInPlane(m_bottom, transformed_sphere);
}