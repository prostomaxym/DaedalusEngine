#pragma once

#include <filesystem>

#include "Macros.h"

namespace Daedalus
{
	// @brief All directory manipulations should be with this class
	class DAEDALUS_API WorkingDirectory
	{
	public:
		static void SetRootDirectory(const std::filesystem::path& path = std::filesystem::path());
		static std::filesystem::path GetRootDirectory();

		static void LoadConfigs();
		static void SaveConfigs();

	private:
		static std::filesystem::path EvaluateStandardRootDirectory();

		WorkingDirectory() = delete;
		WorkingDirectory(const WorkingDirectory&) = delete;
		WorkingDirectory(const WorkingDirectory&&) = delete;
		WorkingDirectory& operator=(const WorkingDirectory&) = delete;
		WorkingDirectory& operator=(const WorkingDirectory&&) = delete;
	};

}