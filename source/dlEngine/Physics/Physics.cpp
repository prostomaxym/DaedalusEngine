#include "dlpch.h"
#include "Physics.h"
#include "PhysicsWorld.h"
#include <cmath>

using namespace Daedalus;

Physics* Physics::s_instance = nullptr;

Physics::Physics()
{
	DL_ASSERT(!s_instance, Log::Categories::Physics, "Only one physics service may be active");
	if (s_instance) std::abort();
	s_instance = this;
}

Physics::~Physics() { s_instance = nullptr; }

Physics& Physics::GetInstance()
{
	DL_ASSERT(s_instance, Log::Categories::Physics, "Physics used outside Engine lifetime");
	if (!s_instance) std::abort();
	return *s_instance;
}

std::unique_ptr<PhysicsWorld> Physics::CreateWorld()
{
	return std::make_unique<PhysicsWorld>(GetInstance().m_gravity);
}

void Physics::SetDefaultGravity(const glm::vec3& gravity)
{
	if (std::isfinite(gravity.x) && std::isfinite(gravity.y) && std::isfinite(gravity.z))
		GetInstance().m_gravity = gravity;
}