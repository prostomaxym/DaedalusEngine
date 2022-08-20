#pragma once

#include <functional>
#include <string>

#include <Daedalus/Core.h>

namespace Daedalus {

enum class EventType
{
	None = 0,

	//Window events
	WindowClose,
	WindowResize,
	WindowFocus,
	WindowLostFocus,
	WindowMoved,

	//Application events
	AppTick,
	AppUpdate,
	AppRender,

	//Keyboard events
	KeyPressed,
	KeyReleased,

	//Mouse events
	MouseButtonPressed,
	MouseButtonReleased,
	MouseMoved,
	MouseScrolled
};

enum EventCategory
{
	None = 0,

	Application = BIT(0),
	Input = BIT(1),
	Keyboard = BIT(2),
	Mouse = BIT(3),
	MouseButton = BIT(4)
};

#define EVENT_CLASS_TYPE(type)	static EventType GetStaticType { return EventType::##type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetNameC() cosnt override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

class DAEDALUS_API Event
{
	friend class EventDispatcher;

public:
	virtual EventType GetEventType() const = 0;
	virtual std::string GetName() const = 0;
	virtual const char* GetNameC() const = 0;
	virtual int GetCategoryFlags() const = 0;

	inline bool IsInCategory(EventCategory category) { return GetCategoryFlags() & category; }

protected:
	bool m_Handled = false;
	
};

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
			m_event.m_handled = func(*(T*)&m_event)
				return true;
		}

		return false;
	}

private:
	Event& m_event;
};

inline std::ostream& operator<<(std::ostream& os, const Event& e)
{
	return os << e.GetName();
}

}