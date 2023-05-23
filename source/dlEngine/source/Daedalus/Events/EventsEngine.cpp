#include "EventsEngine.h"

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
			m_pause_condition.wait(lock, [this] { return !m_paused || !m_running; });

			if (!m_running)
				break;

			if (m_event_queue.empty())
				continue;

			event = std::move(m_event_queue.front());
			m_event_queue.pop_front();

			if (event->GetEllapsedTime() < event->GetTimeToLive())
			{
				event->Dispatch();
			}
		}
	}
}

void Daedalus::EventEngine::StartJob()
{
	m_runner_thread = std::thread(&EventEngine::ProcessEvents, this);
}

void Daedalus::EventEngine::StopJob()
{
	CleanQueue();
}
