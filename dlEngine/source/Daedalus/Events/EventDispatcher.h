#pragma once

#include "Event.h"

namespace Daedalus {

class EventDispatcher
{
	template<typename T>
	using EventFN = std::function<bool(T&)>;
public:

	EventDispatcher(Event& event) : m_event(event) {}

	template<typename T>
	bool Dispatch(EventFN<T> func)
	{
		if (m_event.GetEventType() == T::GetStaticType())
		{
			m_event.SetHandle(func(*(T*)&m_event));
				return true;
		}

		return false;
	}

private:
	Event& m_event;
};

}