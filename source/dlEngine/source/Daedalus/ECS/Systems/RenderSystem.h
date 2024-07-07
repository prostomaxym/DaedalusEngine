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

        void OnStartRuntime() const;
        void OnUpdateRuntime(DeltaTime dt) const;

        void SetCamera(PerspectiveCamera* camera) { m_camera = camera; }
        void SetViewportSize(int width, int height) { m_viewport_width = width; m_viewport_height = height;}

    private:
        void DoStaticLightPass() const;
        void DoDynamicLightPass() const;
        void DoColorPass() const;

        void UpdateStaticLighting() const;
        void UpdateDynamicLighting() const;

        int CountStaticShadowCasters() const;
        int CountDynamicShadowCasters() const;

        BoundingSphere CaltulateSceneBoundingSphere() const;

        entt::registry& m_registry;
        PerspectiveCamera* m_camera { nullptr };
        Scene* m_scene { nullptr };

        int m_viewport_width{ 0 };
		int m_viewport_height{ 0 };
    };
}
