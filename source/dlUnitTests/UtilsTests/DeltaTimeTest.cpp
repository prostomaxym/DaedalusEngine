#include <Utils/DeltaTime.h>

#include <gtest/gtest.h>

#include <chrono>

using Daedalus::DeltaTime;

TEST(DeltaTimeTest, default_is_zero)
{
	const DeltaTime dt;

	EXPECT_EQ(dt.GetTime(), std::chrono::nanoseconds::zero());
	EXPECT_FLOAT_EQ(dt.GetSeconds(), 0.f);
}

TEST(DeltaTimeTest, unit_conversions)
{
	const DeltaTime dt(std::chrono::nanoseconds(1'500'000'000));

	EXPECT_FLOAT_EQ(dt.GetNanoseconds(), 1.5e9f);
	EXPECT_FLOAT_EQ(dt.GetMicroseconds(), 1.5e6f);
	EXPECT_FLOAT_EQ(dt.GetMilliseconds(), 1500.f);
	EXPECT_FLOAT_EQ(dt.GetSeconds(), 1.5f);
}

TEST(DeltaTimeTest, arithmetic_operators)
{
	const DeltaTime a(std::chrono::nanoseconds(300));
	const DeltaTime b(std::chrono::nanoseconds(100));

	EXPECT_EQ((a + b).GetTime(), std::chrono::nanoseconds(400));
	EXPECT_EQ((a - b).GetTime(), std::chrono::nanoseconds(200));

	DeltaTime c = a;
	c += b;
	EXPECT_EQ(c.GetTime(), std::chrono::nanoseconds(400));
	c -= a;
	EXPECT_EQ(c.GetTime(), std::chrono::nanoseconds(100));
}

TEST(DeltaTimeTest, comparison_operators)
{
	const DeltaTime lower(std::chrono::nanoseconds(1));
	const DeltaTime higher(std::chrono::nanoseconds(2));

	EXPECT_TRUE(lower < higher);
	EXPECT_TRUE(lower <= higher);
	EXPECT_TRUE(higher > lower);
	EXPECT_TRUE(higher >= lower);
	EXPECT_TRUE(lower != higher);
	EXPECT_TRUE(lower == DeltaTime(std::chrono::nanoseconds(1)));
	EXPECT_TRUE(lower <= lower);
	EXPECT_TRUE(lower >= lower);
}

