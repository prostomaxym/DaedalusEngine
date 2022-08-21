#pragma once

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
	NoneEvent = 0,

	ApplicationEvent = BITMASK(0),
	InputEvent = BITMASK(1),
	KeyboardEvent = BITMASK(2),
	MouseEvent = BITMASK(3),
	MouseButtonEvent = BITMASK(4)
};

#define EVENT_CLASS_TYPE(type)	static EventType GetStaticType { return EventType::##type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetNameC() cosnt override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

}