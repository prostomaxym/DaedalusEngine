#pragma once

#include "Macros.h"

#include <filesystem>
#include <span>
#include <string>

namespace Daedalus
{
	struct StartupPaths
	{
		std::filesystem::path runtime_root;
		std::filesystem::path path_config;
	};

	class DAEDALUS_API StartupDiscovery
	{
	public:
		// Arguments exclude argv[0]; selection never reads or changes the filesystem.
		[[nodiscard]] static bool Resolve(const std::filesystem::path& executable_path,
			const std::filesystem::path& launch_directory,
			std::span<const std::filesystem::path> arguments, StartupPaths& result, std::string& error);
	};
}
