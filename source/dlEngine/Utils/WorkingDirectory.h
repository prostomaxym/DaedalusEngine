#pragma once

#include "StartupPaths.h"

#include <filesystem>
#include <string>

#include "Macros.h"

namespace Daedalus
{
	// Startup selection is owner-thread-only and precedes application construction.
	class DAEDALUS_API WorkingDirectory
	{
	public:
		[[nodiscard]] static bool SetRootDirectory(const std::filesystem::path& path, std::string& error);
		[[nodiscard]] static bool SetStartupPaths(const StartupPaths& paths, std::string& error);
		static std::filesystem::path GetRootDirectory();
		static std::filesystem::path GetPathConfigPath();

		static void LoadConfigs();
		static void SaveConfigs();

	private:
		static StartupPaths s_startup_paths;

		WorkingDirectory() = delete;
		WorkingDirectory(const WorkingDirectory&) = delete;
		WorkingDirectory(const WorkingDirectory&&) = delete;
		WorkingDirectory& operator=(const WorkingDirectory&) = delete;
		WorkingDirectory& operator=(const WorkingDirectory&&) = delete;
	};

}