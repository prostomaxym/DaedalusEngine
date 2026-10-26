#pragma once

#include "Event.h"

#include <deque>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>

namespace Daedalus
{
	class DAEDALUS_API EventEngine
	{
	public:
		EventEngine();
		~EventEngine() { Stop(); }
		EventEngine(const EventEngine&) = delete;
		EventEngine& operator=(const EventEngine&) = delete;
		EventEngine(EventEngine&&) = delete;
		EventEngine& operator=(EventEngine&&) = delete;
		void Start();
		void Stop();
		void Pause();
		void Resume();
		void DispatchPending(const std::function<void(Event&)>& handler = {});
		
		void CleanQueue();

	private:
		std::deque<std::unique_ptr<Event>> m_event_queue;
		std::deque<std::unique_ptr<Event>> m_dispatch_queue;
		std::mutex m_mutex;
		const std::thread::id m_owner_thread;
		bool m_running{ false };
		bool m_paused{ false };
		bool m_dispatching{ false };

	public:
		bool AddEvent(std::unique_ptr<Event> event);
	};
}