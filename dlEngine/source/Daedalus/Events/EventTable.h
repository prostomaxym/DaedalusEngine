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
	KeyTyped,

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

}