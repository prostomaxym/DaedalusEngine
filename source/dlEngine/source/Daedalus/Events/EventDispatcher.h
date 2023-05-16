#pragma once

#include "Event.h"
#include "EventsEngine.h"

#include <memory>

namespace Daedalus {

	template<typename T>
	class DAEDALUS_API EventWrapper : public Event
	{
	public:
		EVENT_CLASS_TYPE(None)
		EVENT_CLASS_CATEGORY(NoneEvent)
		
		EventWrapper(const std::function<bool(T&)>& func)
			: function(func)
		{
		}

		void Dispatch() override
		{	// Upcast from EventWrapper to Event and then downcast to desired T event type
			// Looks extremely overengineered but it works..
			// Hopefuly entire event system will be refactored to reduce complexity someday but not today
					 
			//              Event
			//    upcast   /     \  downcast
			// EventWrapper       typename T

			function(*static_cast<T*>(static_cast<Event*>(this)));
		}

	private:
		std::function<bool(T&)> function;
	};

	class EventDispatcher
	{
	public:
		EventDispatcher(Event& event)
			: m_event(event)
		{
		}

		// @brief Blocking processing of events
		template<typename T, typename F>
		bool ProcessEvent(const F& func)
		{
			if (m_event.GetEventType() == T::GetStaticType())
			{
				m_event.AddHandle(func(static_cast<T&>(m_event)));
				return true;
			}
			return false;
		}

		// @brief Pass event to EventEngine to process event async in separate thread
		template<typename T, typename F>
		void QueueEvent(const F& func)
		{
			if (m_event.GetEventType() == T::GetStaticType())
			{
				std::function<bool(T&)> func_obj(func);
				EventEngine::AddEvent(std::make_unique<EventWrapper<T>>(func_obj));
			}
		}

	private:
		Event& m_event;
	};
}