#pragma once

#include "EventTable.h"

class EventDispatcher;

namespace Daedalus {

class DAEDALUS_API Event
{
	friend class EventDispatcher;

public:
	virtual EventType GetEventType() const = 0;
	virtual std::string GetInfo() const = 0;
	virtual const char* GetNameC() const = 0;
	virtual int GetCategoryFlags() const = 0;

	inline bool IsInCategory(EventCategory category) { return GetCategoryFlags() & category; }

protected:
	bool m_handled = false;
	
};

inline std::ostream& operator<<(std::ostream& os, const Event& e)
{
	return os << e.GetInfo();
}

}