#pragma once

#include "EventTable.h"
#include "Daedalus/Utils/DeltaTime.h"
#include "Daedalus/Utils/Timer.h"

#include <chrono>
#include <ostream>
#include <string>

namespace Daedalus 
{

	class EventDispatcher;

#define EVENT_CLASS_TYPE(type)	static EventType GetStaticType() { return EventType::type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetNameC() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

	class DAEDALUS_API Event
	{
	public:
		virtual EventType GetEventType() const = 0;
		virtual std::string GetInfo() const { return {}; }
		virtual const char* GetNameC() const = 0;
		virtual int GetCategoryFlags() const = 0;
		virtual void Dispatch() {}; //Used for quenue event engine

		inline void AddHandle(bool handle) { m_handled |= handle; }
		inline void SetHandle(bool handle) { m_handled = handle; }
		inline bool IsInCategory(EventCategory category) { return GetCategoryFlags() & category; }
		inline bool IsHandled() { return m_handled; }

		DeltaTime GetTimeToLive() const { return m_time_to_live; }
		DeltaTime GetEllapsedTime() const { return m_timer.GetEllapsedTime(); }
		void StartTimer() { m_timer.StartTimer(); }

	protected:
		bool m_handled{ false };

		//Used for queue, not used for blocking events
		DeltaTime m_time_to_live{};
		Timer m_timer{};
	};

	inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.GetInfo();
	}

}