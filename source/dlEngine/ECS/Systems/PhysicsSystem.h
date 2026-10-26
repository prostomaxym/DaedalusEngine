#pragma once

#include "Macros.h"
#include "Utils/DeltaTime.h"
#include "Physics/PhysicsWorld.h"

#include <entt/entt.hpp>

namespace Daedalus
{
    class Scene;

    class DAEDALUS_API PhysicsSystem
    {
    public:
        PhysicsSystem(entt::registry& reg, Scene* scene);

        void OnStartRuntime() const;
        void OnStopRuntime() const;
        void OnUpdateRuntime(DeltaTime dt) const;
        PhysicsWorld& GetWorld() { return *m_world; }

    private:
        entt::registry& m_registry;
        Scene* m_scene { nullptr };
        std::unique_ptr<PhysicsWorld> m_world;
    };
}
