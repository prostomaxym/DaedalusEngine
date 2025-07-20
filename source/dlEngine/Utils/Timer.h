#pragma once

#include "DeltaTime.h"

#include <chrono>

namespace Daedalus
{
	class Timer
	{
	public:
		// Create and start counting
		Timer() { StartTimer(); }

		// Reset time and start counting
		void StartTimer() { m_begin_time = std::chrono::steady_clock::now(); }

		DeltaTime GetEllapsedTime() const
		{
			return DeltaTime(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - m_begin_time));
		}

	private:
		std::chrono::steady_clock::time_point m_begin_time{};
	};
}