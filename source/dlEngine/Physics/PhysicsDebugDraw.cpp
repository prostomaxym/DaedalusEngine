#include "dlpch.h"
#include "PhysicsWorldInternal.h"
#include "Debug/Log.h"

using namespace Daedalus;
using namespace Daedalus::PhysicsDetail;

namespace
{
	class DebugDrawer final : public btIDebugDraw
	{
	public:
		explicit DebugDrawer(const PhysicsWorld::DebugLineCallback& line) : m_line(line) {}
		void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override
		{
			if (m_lines++ < 65536) m_line(FromBullet(from), FromBullet(to), FromBullet(color));
		}
		void drawContactPoint(const btVector3& point, const btVector3& normal, btScalar distance, int, const btVector3& color) override
		{
			drawLine(point, point + normal * (distance + 0.1f), color);
		}
		void reportErrorWarning(const char* warning) override
		{
			Log::Write(Log::Levels::Warn, Log::Categories::Physics, "Bullet debug draw: {}", warning);
		}
		void draw3dText(const btVector3&, const char*) override {}
		void setDebugMode(int mode) override { m_mode = mode; }
		int getDebugMode() const override { return m_mode; }

	private:
		const PhysicsWorld::DebugLineCallback& m_line;
		size_t m_lines = 0;
		int m_mode = DBG_DrawWireframe | DBG_DrawConstraints | DBG_DrawConstraintLimits | DBG_DrawContactPoints;
	};
}

void PhysicsWorld::DrawDebug(const DebugLineCallback& line) const
{
	m_impl->CheckThread();
	if (!line || m_impl->m_world.getDebugDrawer()) return;
	DebugDrawer drawer(line);
	m_impl->m_world.setDebugDrawer(&drawer);
	m_impl->m_world.debugDrawWorld();
	m_impl->m_world.setDebugDrawer(nullptr);
}
