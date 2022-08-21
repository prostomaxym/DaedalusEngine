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
	None = 0,

	Application = BITMASK(0),
	Input = BITMASK(1),
	Keyboard = BITMASK(2),
	Mouse = BITMASK(3),
	MouseButton = BITMASK(4)
};

#define EVENT_CLASS_TYPE(type)	static EventType GetStaticType { return EventType::##type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetNameC() cosnt override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

}