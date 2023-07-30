#pragma once

#include "DeltaTime.h"

#include <thread>

namespace Daedalus {

	class FPSLocker
	{
	public:

		static void LockFps(int desired_fps, const DeltaTime& elapsed_time)
		{
			const auto desired_frametime = GetLockedFrameTime(desired_fps);

			const auto sleep_time = desired_frametime - elapsed_time;

			const auto sleep_nanos = sleep_time.GetTime();
			if (sleep_nanos.count() > 0)
			{
				std::this_thread::sleep_for(sleep_nanos);
			}
		}

		static DeltaTime GetLockedFrameTime(int fps)
		{
			return DeltaTime(std::chrono::nanoseconds(1000000000 / fps));
		}
	};
}
