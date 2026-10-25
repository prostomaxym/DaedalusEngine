#pragma once

#include "DeltaTime.h"
#include "Platform/Platform.h"

#include <thread>

namespace Daedalus {

	class FPSLocker
	{
	public:

		// Sleep CPU for desired frame time
		static void LockFpsThreadSleep(int desired_fps, const DeltaTime& elapsed_time)
		{
			if (desired_fps <= 0)
				return;

			const auto desired_frametime = GetLockedFrameTime(desired_fps);

			const auto sleep_time = desired_frametime - elapsed_time;

			const auto sleep_nanos = sleep_time.GetTime();
			if (sleep_nanos.count() <= 0)
				return;

			// OS sleep overshoots by up to ~1ms+, so sleep only part of the time and spin for the rest
			const auto deadline = std::chrono::steady_clock::now() + sleep_nanos;
			constexpr std::chrono::nanoseconds spin_margin = std::chrono::milliseconds(2);

			if (sleep_nanos > spin_margin)
			{
				Platform::PreciseThreadSleep((sleep_nanos - spin_margin).count());
			}

			while (std::chrono::steady_clock::now() < deadline)
			{
			}
		}

		// Does blank job. Provides 100% precise frame time, but increases CPU load
		static void LockFpsBusyWait(int desired_fps, const DeltaTime& elapsed_time)
		{
			if (desired_fps <= 0)
				return;

			const auto desired_frametime = GetLockedFrameTime(desired_fps);
			const auto remaining_time = desired_frametime - elapsed_time;

			if (remaining_time.GetTime().count() <= 0)
				return;

			const auto deadline = std::chrono::steady_clock::now() + remaining_time.GetTime();
			while (std::chrono::steady_clock::now() < deadline)
			{
			}
		}

		static DeltaTime GetLockedFrameTime(int fps)
		{
			return DeltaTime(std::chrono::nanoseconds(1000000000 / fps));
		}
	};
}
