#include <Threads/ThreadJoiner.h>

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

using Daedalus::ThreadJoiner;

TEST(ThreadJoinerTest, joins_all_threads_on_destruction)
{
	std::atomic<int> done{ 0 };
	std::vector<std::thread> threads;

	{
		for (int i = 0; i < 4; ++i)
			threads.emplace_back([&done] { ++done; });

		ThreadJoiner joiner(threads);
	}

	EXPECT_EQ(done, 4);
	for (const auto& t : threads)
		EXPECT_FALSE(t.joinable());
}

TEST(ThreadJoinerTest, handles_empty_vector)
{
	std::vector<std::thread> threads;
	EXPECT_NO_THROW({ ThreadJoiner joiner(threads); });
}

TEST(ThreadJoinerTest, skips_already_joined_threads)
{
	std::vector<std::thread> threads;
	threads.emplace_back([] {});
	threads[0].join();
	EXPECT_NO_THROW({ ThreadJoiner joiner(threads); });
}
