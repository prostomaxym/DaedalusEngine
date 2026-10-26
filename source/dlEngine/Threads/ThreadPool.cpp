#include "dlpch.h"

#include "ThreadPool.h"

using namespace Daedalus;

ThreadPool::ThreadPool(unsigned int n_threads) :
	m_workers(n_threads),
	m_joiner(m_workers)
{
	for (unsigned int i = 0; i < n_threads; ++i)
	{
		try
		{
			m_workers[i] = std::thread(&ThreadPool::DoWork, this);
		}
		catch (...)
		{
			RequestStop();
			throw;
		}
	}
}

ThreadPool::~ThreadPool()
{
	RequestStop();
}

void ThreadPool::RequestStop()
{
	{
		// Serialize the predicate change with workers entering the condition-variable wait.
		std::lock_guard<std::mutex> lock(m_mtx);
		m_done = true;
	}
	m_cv.notify_all();
}

void ThreadPool::DoWork()
{
	while (true)
	{
		std::unique_lock<std::mutex> ul(m_mtx);
		m_cv.wait(ul, [this]() {return !m_q.empty() || m_done; });
		if (m_done)
			return;

		function_wrapper func = std::move(m_q.front());
		m_q.pop();
		ul.unlock();

		// A throwing task must not terminate the worker thread (and the process via std::terminate).
		try
		{
			func();
		}
		catch (const std::exception& e)
		{
			Log::Write(Log::Levels::Error, Log::Categories::EngineCore, "ThreadPool task threw an exception: {}", e.what());
		}
		catch (...)
		{
			Log::Write(Log::Levels::Error, Log::Categories::EngineCore, "ThreadPool task threw an unknown exception");
		}
	}
}
