#pragma once

#include "Macros.h"
#include "Utils/DeltaTime.h"

#include <entt/entt.hpp>

namespace Daedalus
{
    class Scene;

    class DAEDALUS_API PhysicsSystem
    {
    public:
        PhysicsSystem(entt::registry& reg, Scene* scene) : m_registry(reg), m_scene(scene) {}

        void OnStartRuntime() const;
        void OnUpdateRuntime(DeltaTime dt) const;

    private:
        entt::registry& m_registry;
        Scene* m_scene { nullptr };
    };
}
