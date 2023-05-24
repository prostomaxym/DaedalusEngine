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
		void StartTimer() { m_begin_time = std::chrono::high_resolution_clock::now(); }

		DeltaTime GetEllapsedTime() const
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_begin_time);
		}

	private:
		std::chrono::high_resolution_clock::time_point m_begin_time{};
	};
}