#pragma once

#include "Event.h"

namespace Daedalus {

class DAEDALUS_API MouseMovedEvent : public Event
{
public:
	EVENT_CLASS_TYPE(MouseMoved)
	EVENT_CLASS_CATEGORY(MouseEvent | InputEvent)
public:
	MouseMovedEvent(float x, float y) : m_mouseX(x), m_mouseY(y) {}

	inline float GetX() const { return m_mouseX; }
	inline float GetY() const { return m_mouseY; }

	std::string GetInfo() const override
	{
		return "MouseMovedEvent: " + std::to_string(m_mouseX) + ", " + std::to_string(m_mouseY);
	}

private:
	float m_mouseX;
	float m_mouseY;
};


class DAEDALUS_API MouseScrolledEvent : public Event
{
public:
	EVENT_CLASS_TYPE(MouseScrolled)
	EVENT_CLASS_CATEGORY(MouseEvent | InputEvent)
public:
	MouseScrolledEvent(float offsetX, float offsetY) : m_offsetX(offsetX), m_offsetY(offsetY) {}

	inline int GetOffsetX() const { return m_offsetX; }
	inline int GetOffsetY() const { return m_offsetY; }

	std::string GetInfo() const override
	{
		return "MouseScrolledEvent: " + std::to_string(m_offsetX) + ", " + std::to_string(m_offsetY);
	}
	
private:
	float m_offsetX;
	float m_offsetY;
};


class DAEDALUS_API MouseButtonEvent : public Event
{
public:
	EVENT_CLASS_CATEGORY(MouseEvent | InputEvent)
public:
	inline int GetMouseButton() const { return m_button; }

protected:
	MouseButtonEvent(int button) : m_button(button) {}
	int m_button;
};


class DAEDALUS_API MouseButtonPressedEvent : public MouseButtonEvent
{
public:
	EVENT_CLASS_TYPE(MouseButtonPressed)
public:
	MouseButtonPressedEvent(int button) : MouseButtonEvent(button) {}

	std::string GetInfo() const override
	{
		return "MouseButtonPressedEvent: " + std::to_string(m_button);
	}
};


class DAEDALUS_API MouseButtonReleasedEvent : public MouseButtonEvent
{
public:
	EVENT_CLASS_TYPE(MouseButtonReleased)
public:
	MouseButtonReleasedEvent(int button) : MouseButtonEvent(button) {}

	std::string GetInfo() const override
	{
		return "MouseButtonReleasedEvent: " + std::to_string(m_button);
	}
};

}