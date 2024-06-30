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

    struct BoundingSphere
    {
        glm::vec3 position{ 0.f,0.f,0.f };
        float radius = 0.f;
    };

    class Frustum
    {
    public:
        Frustum() = default;
        static Frustum CalculateFrustum(const glm::mat4& view_projection);

        bool SphereInFrustum(const BoundingSphere& sphere, const glm::mat4& transform) const;

    //private:
        glm::vec4 m_right;
        glm::vec4 m_left;
        glm::vec4 m_top;
        glm::vec4 m_bottom;
        glm::vec4 m_near;
        glm::vec4 m_far;
    };
}