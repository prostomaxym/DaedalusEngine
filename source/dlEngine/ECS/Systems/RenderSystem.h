#pragma once

#include "Macros.h"
#include "Utils/DeltaTime.h"

#include <entt/entity/fwd.hpp>
#include <glm/mat4x4.hpp>
#include <memory>
#include <vector>

namespace Daedalus
{
    class Scene;
    class Camera;
    class LightSource;
    struct BoundingSphere;
    struct AABB;
    struct TransformComponent;
    class Model;

    class DAEDALUS_API RenderSystem
    {
    public:
        RenderSystem(entt::registry& reg, Scene* scene);
        ~RenderSystem();

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
        mutable std::vector<TransformComponent> m_transform_inputs;
        mutable std::vector<glm::mat4> m_transform_matrices;
        mutable std::vector<std::shared_ptr<Model>> m_models;
    };
}
