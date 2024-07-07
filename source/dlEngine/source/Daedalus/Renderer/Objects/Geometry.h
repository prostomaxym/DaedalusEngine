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

        BoundingSphere() = default;
        BoundingSphere(const glm::vec3& pos, float radius);

        BoundingSphere GetTransformedSphere(const glm::mat4& matrix) const;
        static BoundingSphere CalculateCommonBoundingSphere(const std::vector<BoundingSphere>& spheres);
    };

    class Frustum
    {
    public:
        Frustum() = default;
        static Frustum CalculateFrustum(const glm::mat4& projection_view);

        bool SphereInFrustum(const BoundingSphere& sphere, const glm::mat4& transform) const;

    private:
        std::array<glm::vec4, 6> m_planes;

        enum Planes
        {
            Left,
            Right,
            Bottom,
            Top,
            Near,
            Far
        };
    };
}