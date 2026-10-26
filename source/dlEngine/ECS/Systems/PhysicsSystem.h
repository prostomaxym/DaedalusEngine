#pragma once

#include "Macros.h"
#include "Utils/DeltaTime.h"

#include <entt/entity/fwd.hpp>
#include <memory>

namespace Daedalus
{
    class Scene;
    class PhysicsWorld;

    class DAEDALUS_API PhysicsSystem
    {
    public:
        PhysicsSystem(entt::registry& reg, Scene* scene);
        ~PhysicsSystem();
        PhysicsSystem(PhysicsSystem&& other) noexcept;

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
