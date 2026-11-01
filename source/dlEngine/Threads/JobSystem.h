#pragma once

#include "Macros.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <span>
#include <thread>

namespace Daedalus
{
	class JobSystem;

	class DAEDALUS_API JobHandle
	{
	public:
		JobHandle() = default;
		[[nodiscard]] bool IsComplete() const;
		[[nodiscard]] bool HasFailed() const;

	private:
		friend class JobSystem;
		struct State;
		explicit JobHandle(std::shared_ptr<State> state);
		std::shared_ptr<State> m_state;
	};

	enum class JobAffinity { Worker, MainThread };

	class DAEDALUS_API JobSystem
	{
	public:
		explicit JobSystem(unsigned int worker_count = std::thread::hardware_concurrency());
		~JobSystem();
		JobSystem(const JobSystem&) = delete;
		JobSystem& operator=(const JobSystem&) = delete;
		JobSystem(JobSystem&&) = delete;
		JobSystem& operator=(JobSystem&&) = delete;

		[[nodiscard]] JobHandle Submit(std::function<void()> task,
			std::span<const JobHandle> dependencies = {}, JobAffinity affinity = JobAffinity::Worker);
		[[nodiscard]] JobHandle WhenAll(std::span<const JobHandle> jobs);
		[[nodiscard]] JobHandle ParallelFor(size_t count, std::function<void(size_t)> task, size_t grain_size = 64);

		// Owner-thread only. Wait pumps main-thread work; workers must express dependencies instead.
		[[nodiscard]] bool Wait(const JobHandle& job);
		void WaitIdle();
		void ProcessMainThread();
		// Reject new submissions and drain all accepted jobs before their captured resources are released.
		void Stop();
		[[nodiscard]] size_t GetWorkerCount() const;
		[[nodiscard]] bool IsAcceptingJobs() const;

	private:
		struct State;
		std::unique_ptr<State> m_state;
	};
}
