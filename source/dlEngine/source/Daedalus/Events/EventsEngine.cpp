#include "EventsEngine.h"

#include <thread>

using namespace Daedalus;

std::deque<std::unique_ptr<Event>> EventEngine::s_event_queue;
bool EventEngine::s_running = false;
std::thread EventEngine::s_thread;

void EventEngine::CleanQueue()
{
	if (s_running)
	{
		s_running = false;
		s_thread.join();
		s_event_queue.clear();
		s_running = true;
	}
	else
	{
		s_event_queue.clear();
	}
}

void EventEngine::Start()
{
	s_running = true;
	s_thread = std::thread(ProcessEvents);
}

void EventEngine::Stop()
{
	s_running = false;
	CleanQueue();
	s_thread.join();
}

void EventEngine::ProcessEvents()
{
	while (s_running)
	{
		if (s_event_queue.empty())
			continue;

		auto& event = s_event_queue.front();

		if (event->GetEllapsedTime() < event->GetTimeToLive())
		{			
			event->Dispatch();		
		}	

		s_event_queue.pop_front();
	}
}
