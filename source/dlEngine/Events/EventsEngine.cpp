#include "EventsEngine.h"
#include "GamepadEvent.h"

using namespace Daedalus;

void EventEngine::CleanQueue()
{
	std::lock_guard<std::mutex> lock(m_runner_mutex);
	m_event_queue.clear();
}

void EventEngine::ProcessEvents()
{
	while (m_running)
	{
		std::unique_ptr<Event> event;

		{
			std::unique_lock<std::mutex> lock(m_runner_mutex);
			m_pause_condition.wait(lock, [this] { return !m_running || (!m_paused && !m_event_queue.empty()); });

			if (!m_running)
				break;

			event = std::move(m_event_queue.front());
			m_event_queue.pop_front();
		}

		// Dispatch outside the lock so handlers can queue new events
		if (event->GetEllapsedTime() < event->GetTimeToLive())
		{
			event->Dispatch();
		}
	}
}

void Daedalus::EventEngine::OnStart()
{
	m_runner_thread = std::thread(&EventEngine::ProcessEvents, this);
}

void Daedalus::EventEngine::OnStop()
{
	CleanQueue();
}

void Daedalus::EventEngine::OnResume()
{
	// Called by ThreadRunner::Resume() with m_runner_mutex held
	for (auto& event : m_event_queue)
		event->StartTimer();
}
