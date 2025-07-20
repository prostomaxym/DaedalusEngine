#pragma once

#include <deque>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace Daedalus
{
	// @brief class for running some process/engine on separate thread
	class DAEDALUS_API ThreadRunner
	{
	public:
		ThreadRunner() : m_running(false), m_paused(false) {}
		virtual ~ThreadRunner() = default;  //Make sure that child call Stop() in its destructor

		void Start();
		void Stop();
		void Pause();
		void Resume();

	protected:
		// Override in child to specify needed behavior 
		virtual void OnStart() {};
		virtual void OnStop() {};
		virtual void OnPause() {};
		virtual void OnResume() {};

		std::atomic<bool> m_running;
		std::atomic<bool> m_paused;
		std::thread m_runner_thread;
		std::mutex m_runner_mutex;
		std::condition_variable m_pause_condition;
	};
}