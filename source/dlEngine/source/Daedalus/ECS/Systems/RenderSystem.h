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

        void UpdateStaticLighting();
        void UpdateDynamicLighting();

        void OnUpdateRuntime(DeltaTime dt);
        void SetCamera(PerspectiveCamera* camera) { m_camera = camera; }
        void SetViewportSize(int width, int height) { m_viewport_width = width; m_viewport_height = height;}

    private:
        entt::registry& m_registry;
        PerspectiveCamera* m_camera { nullptr };
        Scene* m_scene { nullptr };

        int m_viewport_width{ 0 };
		int m_viewport_height{ 0 };
    };
}
