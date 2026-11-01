#include "dlpch.h"
#include "JobSystem.h"
#include "ThreadPool.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>

using namespace Daedalus;

struct JobHandle::State
{
	std::atomic<bool> m_complete{ false };
	std::atomic<bool> m_failed{ false };
	std::shared_ptr<int> m_scheduler;
	std::function<void()> m_task;
	JobAffinity m_affinity = JobAffinity::Worker;
	size_t m_dependencies = 0;
	std::vector<std::shared_ptr<State>> m_dependents;
};

JobHandle::JobHandle(std::shared_ptr<State> state) : m_state(std::move(state)) {}
bool JobHandle::IsComplete() const { return !m_state || m_state->m_complete.load(std::memory_order_acquire); }
bool JobHandle::HasFailed() const { return m_state && m_state->m_failed.load(std::memory_order_acquire); }

struct JobSystem::State
{
	const std::thread::id m_owner = std::this_thread::get_id();
	const std::shared_ptr<int> m_identity = std::make_shared<int>(0);
	std::mutex m_mutex;
	std::condition_variable m_cv;
	std::deque<std::shared_ptr<JobHandle::State>> m_main_queue;
	size_t m_pending = 0;
	bool m_stopping = false;
	// The pool joins before the scheduler state used by its tasks is destroyed.
	ThreadPool m_pool;

	explicit State(unsigned int worker_count) : m_pool((std::max)(1u, worker_count)) {}

	void CheckOwner() const
	{
		DL_ASSERT(std::this_thread::get_id() == m_owner, Log::Categories::EngineCore, "JobSystem waits and main-thread processing require the owner thread");
		if (std::this_thread::get_id() != m_owner) std::abort();
	}

	// Called with m_mutex held. Waiting jobs never occupy worker threads.
	void Schedule(const std::shared_ptr<JobHandle::State>& job)
	{
		if (job->m_affinity == JobAffinity::MainThread)
			m_main_queue.push_back(job);
		else
			m_pool.Submit([this, job] { Execute(job); });
		m_cv.notify_all();
	}

	void Execute(const std::shared_ptr<JobHandle::State>& job)
	{
		if (!job->m_failed.load())
		{
			try
			{
				job->m_task();
			}
			catch (const std::exception& error)
			{
				job->m_failed.store(true);
				Log::Write(Log::Levels::Error, Log::Categories::EngineCore, "Job failed: {}", error.what());
			}
			catch (...)
			{
				job->m_failed.store(true);
				Log::Write(Log::Levels::Error, Log::Categories::EngineCore, "Job failed with an unknown exception");
			}
		}
		// Release task captures before publishing completion.
		job->m_task = {};
		std::lock_guard<std::mutex> lock(m_mutex);
		job->m_complete.store(true, std::memory_order_release);
		for (const auto& dependent : job->m_dependents)
		{
			if (job->m_failed.load()) dependent->m_failed.store(true);
			if (--dependent->m_dependencies == 0) Schedule(dependent);
		}
		job->m_dependents.clear();
		--m_pending;
		m_cv.notify_all();
	}

	bool ProcessOne(std::unique_lock<std::mutex>& lock)
	{
		if (m_main_queue.empty()) return false;
		auto job = std::move(m_main_queue.front());
		m_main_queue.pop_front();
		lock.unlock();
		Execute(job);
		lock.lock();
		return true;
	}
};

JobSystem::JobSystem(unsigned int worker_count) : m_state(std::make_unique<State>(worker_count)) {}
JobSystem::~JobSystem() { Stop(); }
size_t JobSystem::GetWorkerCount() const { return m_state->m_pool.GetNumOfThreads(); }

bool JobSystem::IsAcceptingJobs() const
{
	std::lock_guard<std::mutex> lock(m_state->m_mutex);
	return !m_state->m_stopping;
}

JobHandle JobSystem::Submit(std::function<void()> task, std::span<const JobHandle> dependencies, JobAffinity affinity)
{
	auto job = std::make_shared<JobHandle::State>();
	job->m_scheduler = m_state->m_identity;
	job->m_affinity = affinity;
	std::lock_guard<std::mutex> lock(m_state->m_mutex);
	bool valid = !m_state->m_stopping && static_cast<bool>(task);
	for (const auto& dependency : dependencies)
		if (dependency.m_state && dependency.m_state->m_scheduler != m_state->m_identity) valid = false;
	if (!valid)
	{
		job->m_failed.store(true);
		job->m_complete.store(true);
		return JobHandle(job);
	}
	job->m_task = std::move(task);
	for (const auto& dependency : dependencies)
	{
		if (!dependency.m_state) continue;
		if (dependency.HasFailed()) job->m_failed.store(true);
		if (!dependency.IsComplete())
		{
			++job->m_dependencies;
			dependency.m_state->m_dependents.push_back(job);
		}
	}
	++m_state->m_pending;
	if (job->m_dependencies == 0) m_state->Schedule(job);
	return JobHandle(job);
}

JobHandle JobSystem::WhenAll(std::span<const JobHandle> jobs)
{
	return Submit([] {}, jobs);
}

JobHandle JobSystem::ParallelFor(size_t count, std::function<void(size_t)> task, size_t grain_size)
{
	if (!task || grain_size == 0) return Submit({});
	if (count == 0) return WhenAll({});
	// Bound job allocations by worker count, not element count.
	const size_t chunks = (std::min)(1 + (count - 1) / grain_size, GetWorkerCount());
	const size_t chunk_size = 1 + (count - 1) / chunks;
	auto shared_task = std::make_shared<std::function<void(size_t)>>(std::move(task));
	std::vector<JobHandle> jobs;
	jobs.reserve(chunks);
	for (size_t begin = 0; begin < count;)
	{
		const size_t end = begin + (std::min)(chunk_size, count - begin);
		jobs.push_back(Submit([shared_task, begin, end]
		{
			for (size_t index = begin; index < end; ++index) (*shared_task)(index);
		}));
		begin = end;
	}
	return WhenAll(jobs);
}

bool JobSystem::Wait(const JobHandle& job)
{
	m_state->CheckOwner();
	if (job.m_state && job.m_state->m_scheduler != m_state->m_identity) return false;
	std::unique_lock<std::mutex> lock(m_state->m_mutex);
	while (!job.IsComplete())
	{
		if (!m_state->ProcessOne(lock))
			m_state->m_cv.wait(lock, [&] { return job.IsComplete() || !m_state->m_main_queue.empty(); });
	}
	return !job.HasFailed();
}

void JobSystem::WaitIdle()
{
	m_state->CheckOwner();
	std::unique_lock<std::mutex> lock(m_state->m_mutex);
	while (m_state->m_pending != 0)
	{
		if (!m_state->ProcessOne(lock))
			m_state->m_cv.wait(lock, [&] { return m_state->m_pending == 0 || !m_state->m_main_queue.empty(); });
	}
}

void JobSystem::ProcessMainThread()
{
	m_state->CheckOwner();
	std::unique_lock<std::mutex> lock(m_state->m_mutex);
	// Bound direct frame work; explicit waits inside callbacks may pump additional jobs.
	const size_t count = m_state->m_main_queue.size();
	for (size_t index = 0; index < count; ++index) m_state->ProcessOne(lock);
}

void JobSystem::Stop()
{
	m_state->CheckOwner();
	{
		std::lock_guard<std::mutex> lock(m_state->m_mutex);
		m_state->m_stopping = true;
	}
	WaitIdle();
}
