#include <Utils/FPSLocker.h>

#include <gtest/gtest.h>

#include <chrono>

using Daedalus::DeltaTime;
using Daedalus::FPSLocker;

TEST(FPSLockerTest, locked_frame_time_is_inverse_of_fps)
{
	EXPECT_EQ(FPSLocker::GetLockedFrameTime(100).GetTime(), std::chrono::nanoseconds(10000000));
	EXPECT_EQ(FPSLocker::GetLockedFrameTime(1).GetTime(), std::chrono::nanoseconds(1000000000));
}

TEST(FPSLockerTest, busy_wait_waits_for_remaining_time)
{
	const auto start = std::chrono::steady_clock::now();
	FPSLocker::LockFpsBusyWait(100, DeltaTime(std::chrono::milliseconds(2)));
	const auto waited = std::chrono::steady_clock::now() - start;
	EXPECT_GE(waited, std::chrono::milliseconds(7));
}

TEST(FPSLockerTest, no_wait_when_disabled_or_frame_already_slow)
{
	const auto start = std::chrono::steady_clock::now();
	FPSLocker::LockFpsBusyWait(100, DeltaTime(std::chrono::milliseconds(50)));
	FPSLocker::LockFpsBusyWait(0, DeltaTime());
	FPSLocker::LockFpsThreadSleep(0, DeltaTime());
	FPSLocker::LockFpsThreadSleep(100, DeltaTime(std::chrono::milliseconds(50)));
	EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::milliseconds(5));
}
