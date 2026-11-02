#pragma once

#include <spdlog/sinks/android_sink.h>

namespace Daedalus
{
	inline std::shared_ptr<spdlog::logger> CreateAndroidLogger()
	{
		return spdlog::android_logger_mt("ENGINE", "Daedalus");
	}
}
