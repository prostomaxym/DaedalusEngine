#include <Utils/Random.h>

#include <gtest/gtest.h>

using Daedalus::Random;

TEST(RandomTest, real_distribution_default_range_is_zero_to_one)
{
	for (int i = 0; i < 1000; ++i)
	{
		const double value = Random::UniformRealDistribution<double>();
		EXPECT_GE(value, 0.0);
		EXPECT_LT(value, 1.0);
	}
}

TEST(RandomTest, real_distribution_respects_bounds)
{
	for (int i = 0; i < 1000; ++i)
	{
		const float value = Random::UniformRealDistribution<float>(-5.f, 5.f);
		EXPECT_GE(value, -5.f);
		EXPECT_LE(value, 5.f);
	}
}

TEST(RandomTest, int_distribution_respects_bounds)
{
	for (int i = 0; i < 1000; ++i)
	{
		const int value = Random::UniformIntDistribution<int>(-3, 3);
		EXPECT_GE(value, -3);
		EXPECT_LE(value, 3);
	}
}

TEST(RandomTest, int_distribution_with_equal_bounds_returns_that_value)
{
	EXPECT_EQ(Random::UniformIntDistribution<int>(7, 7), 7);
}

TEST(RandomTest, int_distribution_default_range_compiles_and_runs)
{
	EXPECT_NO_THROW(Random::UniformIntDistribution<unsigned int>());
}

