#pragma once

#include "Macros.h"
#include "Utils/DeltaTime.h"
#include "ECS/UUID.h"

#include <entt/entity/fwd.hpp>
#include <memory>
#include <cstdint>
#include <unordered_map>
#include <span>

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
        bool PrepareJoints(std::span<const UUID> owners = {}) const;

    private:
        void PrepareBodies() const;
        void PrepareCharacters() const;
        void PublishEvents() const;
        entt::registry& m_registry;
        Scene* m_scene { nullptr };
        std::unique_ptr<PhysicsWorld> m_world;
        mutable std::unordered_map<uint64_t, uint64_t> m_body_entities;
    };
}
