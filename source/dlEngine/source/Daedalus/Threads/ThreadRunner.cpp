#include "ThreadRunner.h"

using namespace Daedalus;


void ThreadRunner::Start()
{
	std::lock_guard<std::mutex> lock(m_runner_mutex);
	if (!m_running)
	{
		m_running = true;
		m_paused = false;

		StartJob();
	}
}

void ThreadRunner::Stop()
{
	{
		std::lock_guard<std::mutex> lock(m_runner_mutex);
		if (m_running)
		{
			m_running = false;
			m_paused = false;

		}
	}

	m_pause_condition.notify_all();
	m_runner_thread.join();

	StopJob();
}

void ThreadRunner::Pause()
{
	std::lock_guard<std::mutex> lock(m_runner_mutex);
	m_paused = true;
}

void ThreadRunner::Resume()
{
	std::lock_guard<std::mutex> lock(m_runner_mutex);
	m_paused = false;
	m_pause_condition.notify_all();
}
