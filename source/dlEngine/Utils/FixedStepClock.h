#pragma once

#include "DeltaTime.h"
#include <algorithm>

namespace Daedalus
{
	class FixedStepClock
	{
	public:
		static constexpr auto Step = std::chrono::nanoseconds(16666667);
		static constexpr int MaxSteps = 8;

		int Advance(DeltaTime elapsed)
		{
			// Bound catch-up work after a stall; never feed a large variable step to physics.
			m_accumulated = (std::min)(m_accumulated + (std::max)(elapsed.GetTime(), std::chrono::nanoseconds::zero()), Step * MaxSteps);
			const int steps = static_cast<int>(m_accumulated / Step);
			m_accumulated -= Step * steps;
			return steps;
		}

	private:
		std::chrono::nanoseconds m_accumulated{};
	};
}
