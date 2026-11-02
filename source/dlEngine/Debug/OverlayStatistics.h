#pragma once

#include <cmath>
#include <vector>

namespace Daedalus
{
	namespace OverlayStatistics
	{
		inline float Average(const std::vector<float>& samples)
		{
			double sum = 0.0;
			size_t count = 0;
			for (const auto sample : samples)
			{
				if (std::isfinite(sample) && sample >= 0.f)
				{
					sum += sample;
					++count;
				}
			}
			return count ? static_cast<float>(sum / count) : -1.f;
		}
	}
}
