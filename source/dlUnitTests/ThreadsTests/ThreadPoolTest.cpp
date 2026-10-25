#include <gtest/gtest.h>

#include <Threads/ThreadPool.h>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <utility>
#include <vector>

using namespace Daedalus;

TEST(ThreadPoolTest, creates_requested_number_of_threads)
{
	ThreadPool pool(3);
	EXPECT_EQ(pool.GetNumOfThreads(), 3u);
}

TEST(ThreadPoolTest, submit_and_return_future_gives_result)
{
	ThreadPool pool(2);
	auto future = pool.SubmitAndReturnFuture([] { return 21 * 2; });
	EXPECT_EQ(future.get(), 42);
}

TEST(ThreadPoolTest, all_submitted_tasks_are_executed)
{
	std::atomic<int> counter{ 0 };
	std::vector<std::future<void>> futures;
	{
		ThreadPool pool(4);
		for (int i = 0; i < 100; ++i)
			futures.push_back(pool.SubmitAndReturnFuture([&counter] { ++counter; }));
		for (auto& f : futures)
			f.get();
	}
	EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPoolTest, repeated_construction_executes_first_task)
{
	for (int i = 0; i < 64; ++i)
	{
		ThreadPool pool(1);
		auto future = pool.SubmitAndReturnFuture([i] { return i; });
		EXPECT_EQ(future.get(), i);
	}
}

TEST(ThreadPoolTest, repeated_idle_shutdown_joins_workers)
{
	// Exercise shutdown both while workers start and while they enter their wait.
	for (int i = 0; i < 64; ++i)
	{
		ThreadPool pool(2);
	}
}

TEST(ThreadPoolTest, destruction_joins_running_task)
{
	std::promise<void> task_started;
	std::promise<void> release_task;
	auto release = release_task.get_future();
	auto pool = std::make_unique<ThreadPool>(1);
	auto result = pool->SubmitAndReturnFuture([&task_started, &release]
	{
		task_started.set_value();
		release.wait();
		return 42;
	});
	task_started.get_future().wait();

	std::promise<void> shutdown_started;
	auto shutdown = std::async(std::launch::async,
		[pool = std::move(pool), &shutdown_started]() mutable
		{
			shutdown_started.set_value();
			pool.reset();
		});
	shutdown_started.get_future().wait();
	release_task.set_value();
	shutdown.get();
	EXPECT_EQ(result.wait_for(std::chrono::seconds(0)), std::future_status::ready);
	EXPECT_EQ(result.get(), 42);
}
