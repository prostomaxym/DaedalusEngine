#include "dlpch.h"
#include "PhysicsWorldInternal.h"
#include <algorithm>

using namespace Daedalus;
using namespace Daedalus::PhysicsDetail;

namespace
{
	struct ContactCollector : btCollisionWorld::ContactResultCallback
	{
		const std::unordered_map<const btCollisionObject*, RigidBodyHandle>& m_handles;
		std::map<std::pair<RigidBodyHandle, RigidBodyHandle>, PhysicsContact> m_contacts;
		std::map<std::pair<RigidBodyHandle, RigidBodyHandle>, float> m_depths;
		const btCollisionObject* m_probe = nullptr;

		explicit ContactCollector(const std::unordered_map<const btCollisionObject*, RigidBodyHandle>& handles) : m_handles(handles) {}

		bool needsCollision(btBroadphaseProxy* proxy) const override
		{
			const auto* object = static_cast<const btCollisionObject*>(proxy->m_clientObject);
			return ContactResultCallback::needsCollision(proxy) && (!m_probe ||
				(object != m_probe && m_probe->checkCollideWith(object) && object->checkCollideWith(m_probe)));
		}

		void Add(const btManifoldPoint& point, const btCollisionObject* first, const btCollisionObject* second)
		{
			if (point.getDistance() > 0.f) return;
			const auto a = m_handles.find(first);
			const auto b = m_handles.find(second);
			if (a == m_handles.end() || b == m_handles.end() || a->second == b->second) return;
			const auto key = std::make_pair((std::min)(a->second, b->second), (std::max)(a->second, b->second));
			const auto found = m_depths.find(key);
			if (found != m_depths.end() && found->second <= point.getDistance()) return;
			m_depths[key] = point.getDistance();
			PhysicsContact contact;
			contact.first = key.first;
			contact.second = key.second;
			contact.trigger = !first->hasContactResponse() || !second->hasContactResponse();
			contact.position = FromBullet((point.getPositionWorldOnA() + point.getPositionWorldOnB()) * 0.5f);
			contact.normal = FromBullet(point.m_normalWorldOnB) * (a->second == key.first ? -1.f : 1.f);
			contact.impulse = point.getAppliedImpulse();
			m_contacts[key] = contact;
		}

		btScalar addSingleResult(btManifoldPoint& point, const btCollisionObjectWrapper* first, int, int,
			const btCollisionObjectWrapper* second, int, int) override
		{
			Add(point, first->getCollisionObject(), second->getCollisionObject());
			return 0.f;
		}
	};
}

void PhysicsWorld::Impl::CollectContacts()
{
	ContactCollector collector(m_handles);
	for (int i = 0; i < m_dispatcher.getNumManifolds(); ++i)
	{
		const auto* manifold = m_dispatcher.getManifoldByIndexInternal(i);
		for (int j = 0; j < manifold->getNumContacts(); ++j)
			collector.Add(manifold->getContactPoint(j), manifold->getBody0(), manifold->getBody1());
	}
	// Bullet skips static/static manifolds. Explicit tests also support stationary trigger volumes.
	for (const auto& [handle, body] : m_bodies)
		if (!body.m_body->hasContactResponse())
		{
			const auto* proxy = body.m_body->getBroadphaseHandle();
			collector.m_collisionFilterGroup = proxy->m_collisionFilterGroup;
			collector.m_collisionFilterMask = proxy->m_collisionFilterMask;
			collector.m_probe = body.m_body.get();
			m_world.contactTest(body.m_body.get(), collector);
		}
	m_events.clear();
	for (auto& [key, contact] : collector.m_contacts)
	{
		contact.phase = m_previous_contacts.contains(key) ? ContactPhase::Stay : ContactPhase::Enter;
		m_events.push_back(contact);
	}
	for (const auto& [key, contact] : m_previous_contacts)
		if (!collector.m_contacts.contains(key))
		{
			auto exit = contact;
			exit.phase = ContactPhase::Exit;
			exit.impulse = 0.f;
			m_events.push_back(exit);
		}
	m_previous_contacts = std::move(collector.m_contacts);
}

void PhysicsWorld::RegisterContactCallback(ContactCallback callback)
{
	m_impl->CheckThread();
	m_impl->m_contact_callback = callback ? std::make_shared<ContactCallback>(std::move(callback)) : nullptr;
}

std::span<const PhysicsContact> PhysicsWorld::GetContactEvents() const
{
	m_impl->CheckThread();
	return m_impl->m_events;
}
