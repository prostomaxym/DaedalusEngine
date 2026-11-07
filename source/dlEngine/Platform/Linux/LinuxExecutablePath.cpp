#include "Platform/Platform.h"

#if defined(DL_PLATFORM_LINUX) || defined(DL_PLATFORM_ANDROID)
#include <unistd.h>

#include <cerrno>
#include <system_error>
#include <vector>

namespace Daedalus
{
	bool Platform::GetExecutablePath(std::filesystem::path& path, std::string& error)
	{
		constexpr std::size_t maximum_length = 32768;
		std::vector<char> buffer(256);
		for (;;)
		{
			const auto length = readlink("/proc/self/exe", buffer.data(), buffer.size());
			if (length < 0)
			{
				error = "Cannot locate the executable: " + std::generic_category().message(errno);
				return false;
			}
			if (static_cast<std::size_t>(length) < buffer.size())
			{
				path = std::filesystem::path(std::string(buffer.data(), static_cast<std::size_t>(length))).lexically_normal();
				error.clear();
				return true;
			}
			if (buffer.size() == maximum_length)
			{
				error = "Executable path exceeds the supported path length";
				return false;
			}
			buffer.resize(buffer.size() * 2);
		}
	}
}
#endif
