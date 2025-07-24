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
			m_done = true; // all threads which were started before the exception should finish
			throw;
		}
	}
}

ThreadPool::~ThreadPool()
{
	m_done = true;
	m_cv.notify_all();
}

void ThreadPool::DoWork()
{
	while (!m_done)
	{
		std::unique_lock<std::mutex> ul(m_mtx);
		m_cv.wait(ul, [this]() {return !m_q.empty() || m_done; });
		if (m_done)
			return;

		function_wrapper func = std::move(m_q.front());
		m_q.pop();
		ul.unlock();

		func();
	}
}
