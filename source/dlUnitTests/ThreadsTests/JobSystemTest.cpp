#include <gtest/gtest.h>
#include <Threads/JobSystem.h>

#include <array>
#include <atomic>
#include <future>
#include <stdexcept>
#include <vector>

using namespace Daedalus;

TEST(JobSystemTest, zero_worker_count_uses_one_worker)
{
	JobSystem jobs(0);
	EXPECT_EQ(jobs.GetWorkerCount(), 1u);
	EXPECT_TRUE(jobs.Wait(jobs.Submit([] {})));
}

TEST(JobSystemTest, dependencies_finish_before_dependent_runs)
{
	JobSystem jobs(1);
	std::promise<void> release;
	auto gate = release.get_future().share();
	int value = 0;
	const std::array dependencies{ jobs.Submit([&] { gate.wait(); value = 21; }) };
	auto dependent = jobs.Submit([&] { value *= 2; }, dependencies);
	EXPECT_FALSE(dependent.IsComplete());
	release.set_value();
	ASSERT_TRUE(jobs.Wait(dependent));
	EXPECT_EQ(value, 42);
}

TEST(JobSystemTest, group_counter_waits_for_all_jobs)
{
	JobSystem jobs(4);
	std::atomic<int> count{ 0 };
	std::vector<JobHandle> handles;
	for (int index = 0; index < 100; ++index) handles.push_back(jobs.Submit([&] { ++count; }));
	const std::array dependencies{ jobs.WhenAll(handles) };
	int result = 0;
	auto dependent = jobs.Submit([&] { result = count.load(); }, dependencies);
	ASSERT_TRUE(jobs.Wait(dependent));
	EXPECT_EQ(result, 100);
}

TEST(JobSystemTest, main_thread_job_requires_owner_pump)
{
	JobSystem jobs(2);
	const auto owner = std::this_thread::get_id();
	std::thread::id execution;
	auto task = jobs.Submit([&] { execution = std::this_thread::get_id(); }, {}, JobAffinity::MainThread);
	EXPECT_FALSE(task.IsComplete());
	jobs.ProcessMainThread();
	EXPECT_TRUE(task.IsComplete());
	EXPECT_EQ(execution, owner);
}

TEST(JobSystemTest, wait_pumps_worker_main_worker_chain)
{
	JobSystem jobs(1);
	int value = 0;
	std::thread::id worker;
	const auto owner = std::this_thread::get_id();
	const std::array first{ jobs.Submit([&] { value = 20; worker = std::this_thread::get_id(); }) };
	const std::array second{ jobs.Submit([&] { EXPECT_EQ(std::this_thread::get_id(), owner); ++value; }, first, JobAffinity::MainThread) };
	auto last = jobs.Submit([&] { value *= 2; }, second);
	ASSERT_TRUE(jobs.Wait(last));
	EXPECT_EQ(value, 42);
	EXPECT_NE(worker, owner);
}

TEST(JobSystemTest, pump_defers_newly_queued_main_thread_jobs)
{
	JobSystem jobs(1);
	JobHandle second;
	bool executed = false;
	auto first = jobs.Submit([&]
	{
		second = jobs.Submit([&] { executed = true; }, {}, JobAffinity::MainThread);
	}, {}, JobAffinity::MainThread);
	jobs.ProcessMainThread();
	EXPECT_TRUE(first.IsComplete());
	EXPECT_FALSE(executed);
	jobs.ProcessMainThread();
	EXPECT_TRUE(second.IsComplete());
	EXPECT_TRUE(executed);
}

TEST(JobSystemTest, completed_and_duplicate_dependencies_are_supported)
{
	JobSystem jobs(1);
	auto root = jobs.Submit([] {});
	ASSERT_TRUE(jobs.Wait(root));
	const std::array dependencies{ root, root, JobHandle{} };
	EXPECT_TRUE(jobs.Wait(jobs.Submit([] {}, dependencies)));
}

TEST(JobSystemTest, pending_duplicate_dependencies_are_counted)
{
	JobSystem jobs(1);
	std::promise<void> release;
	auto gate = release.get_future().share();
	auto root = jobs.Submit([gate] { gate.wait(); });
	const std::array dependencies{ root, root };
	auto dependent = jobs.Submit([] {}, dependencies);
	release.set_value();
	EXPECT_TRUE(jobs.Wait(dependent));
}

TEST(JobSystemTest, failure_skips_dependents_and_worker_survives)
{
	JobSystem jobs(1);
	const std::array root{ jobs.Submit([] { throw std::runtime_error("expected job failure"); }) };
	bool executed = false;
	const std::array dependent{ jobs.Submit([&] { executed = true; }, root, JobAffinity::MainThread) };
	auto group = jobs.WhenAll(dependent);
	EXPECT_FALSE(jobs.Wait(group));
	EXPECT_TRUE(group.IsComplete());
	EXPECT_TRUE(group.HasFailed());
	EXPECT_FALSE(executed);
	EXPECT_TRUE(jobs.Wait(jobs.Submit([] {})));
}

TEST(JobSystemTest, parallel_for_processes_each_index_once)
{
	JobSystem jobs(4);
	std::array<int, 257> values{};
	ASSERT_TRUE(jobs.Wait(jobs.ParallelFor(values.size(), [&](size_t index) { ++values[index]; }, 17)));
	for (const int value : values) EXPECT_EQ(value, 1);
}

TEST(JobSystemTest, empty_parallel_for_and_group_complete)
{
	JobSystem jobs(1);
	bool executed = false;
	EXPECT_TRUE(jobs.Wait(jobs.ParallelFor(0, [&](size_t) { executed = true; })));
	EXPECT_TRUE(jobs.Wait(jobs.WhenAll({})));
	EXPECT_FALSE(executed);
}

TEST(JobSystemTest, invalid_tasks_and_grain_size_fail)
{
	JobSystem jobs(1);
	EXPECT_FALSE(jobs.Wait(jobs.Submit({})));
	EXPECT_FALSE(jobs.Wait(jobs.ParallelFor(1, [](size_t) {}, 0)));
}

TEST(JobSystemTest, concurrent_submissions_are_all_executed)
{
	JobSystem jobs(4);
	std::atomic<int> count{ 0 };
	std::vector<std::thread> submitters;
	for (int thread = 0; thread < 4; ++thread)
		submitters.emplace_back([&]
		{
			for (int index = 0; index < 100; ++index)
			{
				auto handle = jobs.Submit([&] { ++count; });
				EXPECT_FALSE(handle.HasFailed());
			}
		});
	for (auto& submitter : submitters) submitter.join();
	jobs.WaitIdle();
	EXPECT_EQ(count.load(), 400);
}

TEST(JobSystemTest, shutdown_drains_dependencies_and_rejects_new_work)
{
	JobSystem jobs(1);
	EXPECT_TRUE(jobs.IsAcceptingJobs());
	int value = 0;
	const std::array root{ jobs.Submit([&] { value = 21; }) };
	auto last = jobs.Submit([&] { value *= 2; }, root, JobAffinity::MainThread);
	jobs.Stop();
	EXPECT_FALSE(jobs.IsAcceptingJobs());
	EXPECT_TRUE(last.IsComplete());
	EXPECT_EQ(value, 42);
	bool executed = false;
	auto rejected = jobs.Submit([&] { executed = true; });
	EXPECT_TRUE(rejected.IsComplete());
	EXPECT_TRUE(rejected.HasFailed());
	EXPECT_FALSE(executed);
	jobs.Stop();
}

TEST(JobSystemTest, destruction_drains_jobs_and_handles_remain_readable)
{
	int value = 0;
	JobHandle last;
	{
		JobSystem jobs(1);
		const std::array root{ jobs.Submit([&] { value = 41; }) };
		last = jobs.Submit([&] { ++value; }, root, JobAffinity::MainThread);
	}
	EXPECT_TRUE(last.IsComplete());
	EXPECT_FALSE(last.HasFailed());
	EXPECT_EQ(value, 42);
}

TEST(JobSystemTest, foreign_dependencies_and_waits_are_rejected)
{
	JobSystem first(1);
	JobSystem second(1);
	const std::array dependencies{ first.Submit([] {}) };
	EXPECT_FALSE(second.Wait(dependencies[0]));
	bool executed = false;
	EXPECT_FALSE(second.Wait(second.Submit([&] { executed = true; }, dependencies)));
	EXPECT_FALSE(executed);
}
