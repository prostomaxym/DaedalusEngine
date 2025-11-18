#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Daedalus
{
    // Opaque handle for rigid bodies. Implementation maps this to Bullet objects.
    using RigidBodyHandle = uint64_t;

    class Physics
    {
    public:
        using CollisionCallback = std::function<void(RigidBodyHandle a, RigidBodyHandle b)>;

        static void Init(const glm::vec3& gravity = glm::vec3(0.0f, -9.81f, 0.0f));
        static void Shutdown();

        // Advance simulation. Call once per frame.
        static void StepSimulation(float deltaTime, int maxSubSteps = 1, float fixedTimeStep = 1.0f / 60.0f);

        // Create simple rigid bodies. Returns an opaque handle owned by the engine.
        // halfExtents is the half-size for the box (x,y,z).
        static RigidBodyHandle CreateBoxRigidBody(float mass, const glm::vec3& halfExtents, const glm::vec3& position, const glm::quat& orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        static RigidBodyHandle CreateSphereRigidBody(float mass, float radius, const glm::vec3& position, const glm::quat& orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

        static void RemoveRigidBody(RigidBodyHandle body);

        static void SetGravity(const glm::vec3& gravity);

        // Get/set transform of a rigid body (returns false if handle invalid).
        static bool GetRigidBodyTransform(RigidBodyHandle body, glm::vec3& outPosition, glm::quat& outOrientation);
        static bool SetRigidBodyTransform(RigidBodyHandle body, const glm::vec3& position, const glm::quat& orientation);

        static void RegisterCollisionCallback(CollisionCallback cb);

    private:
        Physics::Physics() = default;

        struct Impl;
        static std::unique_ptr<Impl> s_impl;
    };
}