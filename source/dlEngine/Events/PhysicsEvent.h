#pragma once

#include "Event.h"
#include "Physics/PhysicsTypes.h"
#include <cstdint>

namespace Daedalus
{
	class DAEDALUS_API ScenePhysicsEvent : public Event
	{
	public:
		EVENT_CLASS_TYPE(PhysicsContact)
		EVENT_CLASS_CATEGORY(PhysicsEvent)

		ScenePhysicsEvent(uint64_t scene, uint64_t first, uint64_t second, const Daedalus::PhysicsContact& contact)
			: m_scene(scene), m_first(first), m_second(second), m_contact(contact) {}
		uint64_t GetScene() const { return m_scene; }
		uint64_t GetFirstEntity() const { return m_first; }
		uint64_t GetSecondEntity() const { return m_second; }
		const Daedalus::PhysicsContact& GetContact() const { return m_contact; }

	private:
		uint64_t m_scene;
		uint64_t m_first;
		uint64_t m_second;
		Daedalus::PhysicsContact m_contact;
	};
}
