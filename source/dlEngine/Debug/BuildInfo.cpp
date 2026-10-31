#include "dlpch.h"

#include "BuildInfo.h"

#include <fmt/format.h>

namespace Daedalus
{
	std::string GetBuildInfo()
	{
#if defined(_M_ARM64EC)
		constexpr auto architecture = "arm64ec";
#elif defined(_M_ARM64) || defined(__aarch64__)
		constexpr auto architecture = "arm64";
#elif defined(_M_X64) || defined(__x86_64__)
		constexpr auto architecture = "x64";
#elif defined(_M_IX86) || defined(__i386__)
		constexpr auto architecture = "x86";
#elif defined(_M_ARM) || defined(__arm__)
		constexpr auto architecture = "arm";
#else
		constexpr auto architecture = "unknown";
#endif

		return fmt::format("Application version: {}\nBuild version: {}\nConfiguration: {}\nPlatform: {}\nArchitecture: {}\nCompiler: {}\nDeveloper tools: {}",
			DL_APPLICATION_VERSION, DL_APPLICATION_VERSION, DL_BUILD_CONFIGURATION, DL_BUILD_PLATFORM, architecture,
			DL_BUILD_COMPILER, IsDevBuild() ? "enabled" : "disabled");
	}
}
