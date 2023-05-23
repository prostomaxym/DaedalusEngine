#pragma once

#include "Event.h"
#include "Daedalus/Threads/ThreadRunner.h"

#include <deque>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace Daedalus
{
	class EventEngine : public ThreadRunner
	{
	public:
		EventEngine() : ThreadRunner() {}
		~EventEngine() { Stop(); }
		
		void CleanQueue();

	private:
		void ProcessEvents();
		void StartJob() override;
		void StopJob() override;

		std::deque<std::unique_ptr<Event>> m_event_queue; //replace shared ptr if event queue will be needed outside of this class\thread

	public:
		template<typename EventT>
		void AddEvent(std::unique_ptr<EventT> event)
		{
			event->StartTimer();
			m_event_queue.emplace_back(std::move(event));
		}
	};
}