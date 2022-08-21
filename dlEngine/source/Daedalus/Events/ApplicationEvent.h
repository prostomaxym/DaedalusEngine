#pragma once

#include "Event.h"

namespace Daedalus {

class DAEDALUS_API WindowResizeEvent : public Event
{
public:
	EVENT_CLASS_TYPE(WindowResize)
	EVENT_CLASS_CATEGORY(Application)
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
	EVENT_CLASS_TYPE(WindowCloseEvent)
	Event_CLASS_CATEGORY(Application)
public:
	WindowCloseEvent() {}
};


class DAEDALUS_API AppTickEvent : public Event
{
public:
	EVENT_CLASS_TYPE(AppTickEvent)
	Event_CLASS_CATEGORY(Application)
public:
	AppTickEvent() {}
};


class DAEDALUS_API AppUpdateEvent : public Event
{
public:
	EVENT_CLASS_TYPE(AppUpdateEvent)
	Event_CLASS_CATEGORY(Application)
public:
	AppUpdateEvent() {}
};


class DAEDALUS_API AppRenderEvent : public Event
{
public:
	EVENT_CLASS_TYPE(AppRenderEvent)
	Event_CLASS_CATEGORY(Application)
public:
	AppRenderEvent() {}
};

}