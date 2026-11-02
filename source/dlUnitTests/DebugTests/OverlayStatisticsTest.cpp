#include <gtest/gtest.h>
#include "Debug/OverlayStatistics.h"
#include <limits>

namespace Daedalus
{
	TEST(OverlayStatisticsTest, EmptyHistoryIsUnavailable)
	{
		EXPECT_LT(OverlayStatistics::Average({}), 0.f);
	}

	TEST(OverlayStatisticsTest, ZeroIsAValidMeasurement)
	{
		EXPECT_FLOAT_EQ(OverlayStatistics::Average({ 0.f, 20.f, 10.f }), 10.f);
		EXPECT_FLOAT_EQ(OverlayStatistics::Average({ 0.f, 0.f }), 0.f);
	}

	TEST(OverlayStatisticsTest, IgnoresUnavailableAndNonfiniteSamples)
	{
		EXPECT_FLOAT_EQ(OverlayStatistics::Average({ -1.f, 10.f,
			std::numeric_limits<float>::quiet_NaN(), 20.f,
			std::numeric_limits<float>::infinity() }), 15.f);
	}

	TEST(OverlayStatisticsTest, WarmupDoesNotDiluteMeasurements)
	{
		std::vector<float> samples(100, std::numeric_limits<float>::quiet_NaN());
		samples.back() = 16.f;
		EXPECT_FLOAT_EQ(OverlayStatistics::Average(samples), 16.f);
	}

	TEST(OverlayStatisticsTest, MissingHistoryIsUnavailable)
	{
		EXPECT_LT(OverlayStatistics::Average({ -1.f, std::numeric_limits<float>::quiet_NaN() }), 0.f);
	}
}
