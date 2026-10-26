#include <Utils/FixedStepClock.h>
#include <gtest/gtest.h>

using namespace Daedalus;

TEST(FixedStepClockTest, accumulates_substep_frames)
{
	FixedStepClock clock;
	EXPECT_EQ(clock.Advance(DeltaTime(FixedStepClock::Step / 2)), 0);
	EXPECT_EQ(clock.Advance(DeltaTime(FixedStepClock::Step - FixedStepClock::Step / 2)), 1);
	EXPECT_EQ(clock.Advance(DeltaTime{}), 0);
}

TEST(FixedStepClockTest, bounds_work_after_a_long_stall)
{
	FixedStepClock clock;
	EXPECT_EQ(clock.Advance(DeltaTime(std::chrono::seconds(30))), FixedStepClock::MaxSteps);
	EXPECT_EQ(clock.Advance(DeltaTime{}), 0);
}

TEST(FixedStepClockTest, ignores_negative_elapsed_time)
{
	FixedStepClock clock;
	EXPECT_EQ(clock.Advance(DeltaTime(-FixedStepClock::Step)), 0);
	EXPECT_EQ(clock.Advance(DeltaTime(FixedStepClock::Step)), 1);
}

TEST(FixedStepClockTest, frame_partition_does_not_change_step_count)
{
	FixedStepClock clock;
	int steps = 0;
	for (int i = 0; i < 120; ++i)
		steps += clock.Advance(DeltaTime(FixedStepClock::Step / 2));
	// Preserve the odd nanosecond instead of accumulating rounding error.
	steps += clock.Advance(DeltaTime(std::chrono::nanoseconds(60)));
	EXPECT_EQ(steps, 60);
}
