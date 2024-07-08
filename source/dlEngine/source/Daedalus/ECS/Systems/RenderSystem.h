#pragma once

#include "Daedalus/Renderer/Objects/PerspectiveCamera.h"
#include "Daedalus/Utils/DeltaTime.h"

#include <entt/entt.hpp>

namespace Daedalus
{
    class Scene;

    class RenderSystem
    {
    public:
        RenderSystem(entt::registry& reg, Scene* scene) : m_registry(reg), m_scene(scene) {}

        void OnStartRuntime();
        void OnUpdateRuntime(DeltaTime dt) const;

        void SetCamera(PerspectiveCamera* camera) { m_camera = camera; }
        void SetViewportSize(int width, int height) { m_viewport_width = width; m_viewport_height = height;}

    private:
        void DoLightPass() const;
        void DoColorPass() const;

        void UpdateStaticLighting();
        void UpdateDynamicLighting() const;

        int CountShadowCasters() const;

        BoundingSphere CalculateSceneBoundingSphere() const;
        AABB CalculateSceneBoundingAABB() const;

        entt::registry& m_registry;
        PerspectiveCamera* m_camera { nullptr };
        Scene* m_scene { nullptr };

        std::vector<glm::mat4> m_static_light_space;

        int m_viewport_width{ 0 };
		int m_viewport_height{ 0 };
    };
}
