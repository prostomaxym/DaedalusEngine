#include "Platform/Platform.h"

#ifdef DL_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <system_error>
#include <vector>

namespace Daedalus
{
	bool Platform::GetExecutablePath(std::filesystem::path& path, std::string& error)
	{
		constexpr std::size_t maximum_length = 32768;
		std::vector<wchar_t> buffer(256);
		for (;;)
		{
			const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
			if (length == 0)
			{
				error = "Cannot locate the executable: " + std::system_category().message(static_cast<int>(GetLastError()));
				return false;
			}
			if (length < buffer.size())
			{
				path = std::filesystem::path(std::wstring(buffer.data(), length)).lexically_normal();
				error.clear();
				return true;
			}
			if (buffer.size() == maximum_length)
			{
				error = "Executable path exceeds the supported Windows path length";
				return false;
			}
			buffer.resize(buffer.size() * 2);
		}
	}
}
#endif
