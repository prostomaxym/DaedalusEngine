#include "EventsEngine.h"
#include "GamepadEvent.h"
#include "Debug/Log.h"

using namespace Daedalus;

EventEngine::EventEngine() : m_owner_thread(std::this_thread::get_id())
{
}

void EventEngine::Start()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_running = true;
	m_paused = false;
}

void EventEngine::Stop()
{
	std::deque<std::unique_ptr<Event>> discarded;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_running = false;
		discarded.swap(m_event_queue);
	}
}

void EventEngine::CleanQueue()
{
	std::deque<std::unique_ptr<Event>> discarded;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		discarded.swap(m_event_queue);
	}
}

void EventEngine::Pause()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_paused = true;
}

void EventEngine::Resume()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto& event : m_event_queue)
		event->StartTimer();
	m_paused = false;
}

bool EventEngine::AddEvent(std::unique_ptr<Event> event)
{
	if (!event) return false;
	event->StartTimer();
	std::lock_guard<std::mutex> lock(m_mutex);
	if (!m_running) return false;
	m_event_queue.push_back(std::move(event));
	return true;
}

void EventEngine::DispatchPending(const std::function<void(Event&)>& handler)
{
	DL_ASSERT(std::this_thread::get_id() == m_owner_thread, Log::Categories::Events, "Events must be dispatched on the owning thread");
	if (std::this_thread::get_id() != m_owner_thread || m_dispatching) return;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!m_running || m_paused) return;
		m_dispatch_queue.swap(m_event_queue);
	}
	m_dispatching = true;
	while (!m_dispatch_queue.empty())
	{
		auto event = std::move(m_dispatch_queue.front());
		m_dispatch_queue.pop_front();
		bool dispatch;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			dispatch = m_running && !m_paused;
		}
		if (dispatch && event->GetEllapsedTime() < event->GetTimeToLive())
		{
			event->Dispatch();
			if (handler) handler(*event);
		}
	}
	m_dispatching = false;
}
