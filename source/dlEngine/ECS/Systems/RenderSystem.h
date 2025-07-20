#pragma once

#include "Renderer/Objects/Camera.h"
#include "Renderer/Objects/LightSource.h"
#include "Utils/DeltaTime.h"

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

        void SetCamera(Camera* camera) { m_camera = camera; }
        void SetViewportSize(int width, int height);

        void AddLight(LightSource* light) const;
        void RemoveLight(LightSource* light) const;

    private:
        void RenderSkybox() const;
        void SubmitModels() const;

        BoundingSphere CalculateSceneBoundingSphere() const;
        AABB CalculateSceneBoundingAABB() const;

        entt::registry& m_registry;
        Camera* m_camera { nullptr };
        Scene* m_scene { nullptr };

        int m_viewport_width{ 0 };
		int m_viewport_height{ 0 };
    };
}
