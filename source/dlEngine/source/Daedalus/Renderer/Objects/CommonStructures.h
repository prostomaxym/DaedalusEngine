#pragma once

#include <glm/glm.hpp>

namespace Daedalus {

    struct Vertex
    {
        glm::vec3 position{ 0.f,0.f,0.f };
        glm::vec2 tex_coords{ 0.f,0.f };
        glm::vec3 normals{ 0.f,0.f,0.f };
        glm::vec3 tangent{ 0.f,0.f,0.f };
        glm::vec3 bitangent{ 0.f,0.f,0.f };
    };

    struct Plane
    {
        glm::vec3 normal = { 0.f, 1.f, 0.f };
        float distance{ 0.f };

        Plane() = default;
        Plane(const glm::vec3& p1, const glm::vec3& norm) :
            normal(glm::normalize(norm)),
            distance(glm::dot(normal, p1))
        {}

        float GetSignedDistanceToPlane(const glm::vec3& point) const;
    };

    struct BoundingSphere
    {
        glm::vec3 position{ 0.f,0.f,0.f };
        float radius = 0.f;

        bool IsOnOrForwardPlane(const Plane& plane) const;
    };

    struct Frustum
    {
        Plane top_face;
        Plane bottom_face;

        Plane right_face;
        Plane left_face;

        Plane far_face;
        Plane near_face;

        bool IsInFrustum(const BoundingSphere& sphere, const glm::mat4& transform) const;
    };
}