#include <Threads/ThreadRunner.h>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

using namespace Daedalus;

namespace
{
	class TestRunner : public ThreadRunner
	{
	public:
		~TestRunner() { Stop(); }

		std::atomic<int> starts{ 0 };
		std::atomic<int> stops{ 0 };
		std::atomic<int> pauses{ 0 };
		std::atomic<int> resumes{ 0 };
		std::atomic<int> ticks{ 0 };

	protected:
		void OnStart() override
		{
			++starts;
			m_runner_thread = std::thread([this]
			{
				while (m_running)
				{
					std::unique_lock<std::mutex> lock(m_runner_mutex);
					m_pause_condition.wait(lock, [this] { return !m_paused || !m_running; });
					if (!m_running)
						break;
					++ticks;
					lock.unlock();
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			});
		}
		void OnStop() override { ++stops; }
		void OnPause() override { ++pauses; }
		void OnResume() override { ++resumes; }
	};
}

TEST(ThreadRunnerTest, start_calls_on_start_once)
{
	TestRunner runner;
	runner.Start();
	runner.Start();
	EXPECT_EQ(runner.starts, 1);
	runner.Stop();
}

TEST(ThreadRunnerTest, stop_calls_on_stop_and_joins_thread)
{
	TestRunner runner;
	runner.Start();
	runner.Stop();
	EXPECT_EQ(runner.stops, 1);
}

TEST(ThreadRunnerTest, stop_without_start_does_not_throw)
{
	TestRunner runner;
	EXPECT_NO_THROW(runner.Stop());
}

TEST(ThreadRunnerTest, destroying_never_started_runner_does_not_throw)
{
	EXPECT_NO_THROW({ TestRunner runner; });
}

TEST(ThreadRunnerTest, pause_and_resume_invoke_hooks)
{
	TestRunner runner;
	runner.Start();
	runner.Pause();
	runner.Resume();
	EXPECT_EQ(runner.pauses, 1);
	EXPECT_EQ(runner.resumes, 1);
	runner.Stop();
}

TEST(ThreadRunnerTest, paused_runner_stops_ticking)
{
	TestRunner runner;
	runner.Start();
	std::this_thread::sleep_for(std::chrono::milliseconds(30));
	runner.Pause();
	std::this_thread::sleep_for(std::chrono::milliseconds(20));
	const int paused_ticks = runner.ticks;
	std::this_thread::sleep_for(std::chrono::milliseconds(30));
	EXPECT_EQ(runner.ticks, paused_ticks);
	runner.Resume();
	runner.Stop();
}

TEST(ThreadRunnerTest, can_restart_after_stop)
{
	TestRunner runner;
	runner.Start();
	runner.Stop();
	runner.Start();
	EXPECT_EQ(runner.starts, 2);
	runner.Stop();
}
