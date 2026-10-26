#pragma once

#include "PhysicsWorld.h"

namespace Daedalus
{
    class DAEDALUS_API Physics
    {
    public:
        Physics();
        ~Physics();
        Physics(const Physics&) = delete;
        Physics& operator=(const Physics&) = delete;
        Physics(Physics&&) = delete;
        Physics& operator=(Physics&&) = delete;
        static std::unique_ptr<PhysicsWorld> CreateWorld();
        static void SetDefaultGravity(const glm::vec3& gravity);

    private:
        static Physics& GetInstance();
        static Physics* s_instance; // Non-owning; Engine owns the service.
        glm::vec3 m_gravity{ 0.f, -9.81f, 0.f };
    };
}