#include <Utils/Timer.h>

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

using Daedalus::Timer;

TEST(TimerTest, elapsed_time_is_not_negative)
{
	const Timer timer;

	EXPECT_GE(timer.GetEllapsedTime().GetTime(), std::chrono::nanoseconds::zero());
}

TEST(TimerTest, elapsed_time_covers_sleep)
{
	const Timer timer;
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	EXPECT_GE(timer.GetEllapsedTime().GetTime(), std::chrono::milliseconds(10));
}

TEST(TimerTest, update_delta_time_returns_time_since_last_update)
{
	Timer timer;
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	EXPECT_GE(timer.UpdateDeltaTime().GetTime(), std::chrono::milliseconds(10));
}

TEST(TimerTest, start_timer_restarts_counting)
{
	Timer timer;
	std::this_thread::sleep_for(std::chrono::milliseconds(20));
	const auto before_restart = timer.GetEllapsedTime();
	timer.StartTimer();

	EXPECT_LT(timer.GetEllapsedTime(), before_restart);
}

