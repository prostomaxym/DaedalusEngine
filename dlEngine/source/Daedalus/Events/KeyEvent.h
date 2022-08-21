#pragma once

#include <sstream>

#include "Event.h"

namespace Daedalus {

class DAEDALUS_API KeyEvent : public Event
{
public:
	EVENT_CLASS_CATEGORY(Keyboard | Input)
public:
	inline int GetKeyCode() const { return m_keycode; }

protected:
	KeyEvent(int keycode) : m_keycode(keycode) {}

	int m_keycode;
};


class DAEDALUS_API KeyPressedEvent : public KeyEvent
{
public:
	EVENT_CLASS_TYPE(KeyPressed)
public:
	KeyPressedEvent(int keycode, int repeat_count) : KeyEvent(keycode), m_repeat_count(repeat_count) {}

	inline int GetRepeatCount() const { return m_repeat_count; }
	std::string GetInfo() const override
	{
		return "KeyPressedEvent: "+ std::to_string(m_keycode) + " ("+ std::to_string(m_repeat_count) + " repeats)";
	}

private:
	int m_repeat_count;
};


class DAEDALUS_API KeyReleasedEvent : public KeyEvent
{
public:
	EVENT_CLASS_TYPE(KeyReleased)
public:

	KeyReleasedEvent(int keycode, int repeat_count) : KeyEvent(keycode) {}

	std::string GetName() const override
	{
		return "KeyPressedEvent: " + std::to_string(m_keycode);
	}
};

}