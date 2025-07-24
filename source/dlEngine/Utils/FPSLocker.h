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
			if (sleep_nanos.count() > 0)
			{
				Platform::PreciseThreadSleep(sleep_nanos.count());
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

			const auto start = std::chrono::high_resolution_clock::now();
			while (true)
			{
				const auto now = std::chrono::high_resolution_clock::now();
				const std::chrono::duration<float> waited = now - start;
				if (waited >= remaining_time.GetTime())
					break;
			}
		}

		static DeltaTime GetLockedFrameTime(int fps)
		{
			return DeltaTime(std::chrono::nanoseconds(1000000000 / fps));
		}
	};
}
