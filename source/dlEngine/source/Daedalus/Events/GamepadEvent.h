#pragma once

#include "Event.h"

namespace Daedalus {

//Gamepad button events
class DAEDALUS_API GpadButtonEvent : public Event
{
public:
	EVENT_CLASS_CATEGORY(GamepadButtonEvent | InputEvent)
public:
	inline int GetKeyCode() const { return m_keycode; }

protected:
	GpadButtonEvent(int keycode) : m_keycode(keycode) {}

	int m_keycode;
};


class DAEDALUS_API GpadPressedEvent : public GpadButtonEvent
{
public:
	EVENT_CLASS_TYPE(GamepadPressed)
public:
	GpadPressedEvent(int keycode, int repeat_count) : GpadButtonEvent(keycode), m_repeat_count(repeat_count) {}

	inline int GetRepeatCount() const { return m_repeat_count; }
	std::string GetInfo() const override
	{
		return "GpadPressedEvent: " + std::to_string(m_keycode) + " (" + std::to_string(m_repeat_count) + " repeats)";
	}

private:
	int m_repeat_count;
};


class DAEDALUS_API GpadReleasedEvent : public GpadButtonEvent
{
public:
	EVENT_CLASS_TYPE(GamepadReleased)
public:

	GpadReleasedEvent(int keycode) : GpadButtonEvent(keycode) {}

	std::string GetInfo() const override
	{
		return "GpadReleasedEvent: " + std::to_string(m_keycode);
	}
};
//-----------------------------------------------------------------------------------------------------------------------//


//Gamepad analog events
class DAEDALUS_API GpadAnalogEvent : public Event
{
public:
	EVENT_CLASS_CATEGORY(GamepadAnalogEvent | InputEvent)
public:
	inline int GetKeyCode() const { return m_keycode; }

protected:
	GpadAnalogEvent(int keycode) : m_keycode(keycode) {}
	
	int m_keycode;
};

class DAEDALUS_API GpadStickEvent : public GpadAnalogEvent
{
public:
	EVENT_CLASS_TYPE(GamepadStick)

	enum StickType
	{
		Left,
		Right
	};

public:
	GpadStickEvent(int keycode, float perc_pushed, StickType type) : GpadAnalogEvent(keycode), m_percent_pushed(perc_pushed), m_type(type) {}
	inline int GetPercentagePushed() const { return m_percent_pushed; }
	std::string GetInfo() const override
	{
		std::string type(m_type == StickType::Left ? " -Left" : " -Right");
		return "GpadStickEvent: " + std::to_string(m_keycode) + type +" (" + std::to_string(m_percent_pushed * 100) + "% pushed)";
	}

private:
	float m_percent_pushed;
	StickType m_type;
};

class DAEDALUS_API GpadTriggerEvent : public GpadAnalogEvent
{
public:
	EVENT_CLASS_TYPE(GamepadTrigger)

	enum TriggerType
	{
		Left,
		Right
	};

public:
	GpadTriggerEvent(int keycode, float perc_pushed, TriggerType type) : GpadAnalogEvent(keycode), m_percent_pushed(perc_pushed), m_type(type) {}
	inline int GetPercentagePushed() const { return m_percent_pushed; }
	std::string GetInfo() const override
	{
		std::string type(m_type == TriggerType::Left ? " -Left" : " -Right");
		return "GpadTriggerEvent: " + std::to_string(m_keycode) + type + " (" + std::to_string(m_percent_pushed * 100) + "% pushed)";
	}

private:
	float m_percent_pushed;
	TriggerType m_type;
};
//-----------------------------------------------------------------------------------------------------------//

}