#pragma once

#include <filesystem>

#include "Daedalus/Core/Core.h"

namespace Daedalus
{
	// @brief All directory manipulations should be with this class
	class DAEDALUS_API WorkingDirectory
	{
	public:
		static void SetRootDirectory(const std::filesystem::path& path = std::filesystem::path());
		static std::filesystem::path GetRootDirectory();
		static std::filesystem::path GetAssetsDirectory();
		static std::filesystem::path GetShaderDirectory();

	private:
		static std::filesystem::path EvaluateStandardRootDirectory();

		WorkingDirectory() = delete;
		WorkingDirectory(const WorkingDirectory&) = delete;
		WorkingDirectory(const WorkingDirectory&&) = delete;
		WorkingDirectory& operator=(const WorkingDirectory&) = delete;
		WorkingDirectory& operator=(const WorkingDirectory&&) = delete;
	};

}