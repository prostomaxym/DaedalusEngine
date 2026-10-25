#include <gtest/gtest.h>

#include <Threads/ThreadPool.h>

#include <atomic>
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
