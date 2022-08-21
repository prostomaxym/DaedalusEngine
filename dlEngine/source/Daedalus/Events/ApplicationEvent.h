#pragma once

#include "Event.h"

namespace Daedalus {

class DAEDALUS_API WindowResizeEvent : public Event
{
public:
	EVENT_CLASS_TYPE(WindowResize)
	EVENT_CLASS_CATEGORY(ApplicationEvent)
public:
	WindowResizeEvent(int width, int height) : m_width(width), m_height(height) {}

	inline int GetWidth() const { return m_width; }
	inline int GetHeight() const { return m_height; }

	std::string GetName() const override
	{
		return "WindowReiseEvent: " + std::to_string(m_width) + ", " + std::to_string(m_height);
	}

private:
	int m_width;
	int m_height;
};


class DAEDALUS_API WindowCloseEvent : public Event
{
public:
	EVENT_CLASS_TYPE(WindowClose)
	Event_CLASS_CATEGORY(ApplicationEvent)
public:
	WindowCloseEvent() {}
};


class DAEDALUS_API AppTickEvent : public Event
{
public:
	EVENT_CLASS_TYPE(AppTick)
	Event_CLASS_CATEGORY(ApplicationEvent)
public:
	AppTickEvent() {}
};


class DAEDALUS_API AppUpdateEvent : public Event
{
public:
	EVENT_CLASS_TYPE(AppUpdate)
	Event_CLASS_CATEGORY(ApplicationEvent)
public:
	AppUpdateEvent() {}
};


class DAEDALUS_API AppRenderEvent : public Event
{
public:
	EVENT_CLASS_TYPE(AppRender)
	Event_CLASS_CATEGORY(ApplicationEvent)
public:
	AppRenderEvent() {}
};

}