#pragma once

#include "Event.h"

#include <deque>
#include <functional>
#include <thread>

namespace Daedalus
{
	class EventEngine
	{
	public:
		static void CleanQueue();

		static void Start();
		static void Stop();
		static bool IsRunning() { return s_running; }

		template<typename EventT>
		static void AddEvent(std::unique_ptr<EventT> event)
		{
			event->StartTimer();
			s_event_queue.emplace_back(std::move(event));
		}
			
	private:
		static void ProcessEvents();

		static std::deque<std::unique_ptr<Event>> s_event_queue;
		static bool s_running;
		static std::thread s_thread;
	};
}