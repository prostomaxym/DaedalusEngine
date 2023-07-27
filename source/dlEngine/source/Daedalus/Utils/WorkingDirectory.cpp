#include "WorkingDirectory.h"

#include "Daedalus/Debug/Log.h"

using namespace Daedalus;

namespace
{
	constexpr auto bin_dir = "bin";
	constexpr auto changedir_back = "../";
	constexpr auto shaders_dir = "shaders";
	constexpr auto assets_dir = "assets";
}

void WorkingDirectory::SetRootDirectory(const std::filesystem::path& path)
{
	const auto& current_path = !path.empty() ? path : EvaluateStandardRootDirectory();
	std::filesystem::current_path(current_path);
}

std::filesystem::path WorkingDirectory::GetRootDirectory()
{
	return std::filesystem::current_path();
}

std::filesystem::path WorkingDirectory::GetAssetsDirectory()
{
	auto path = std::filesystem::current_path();
	path.append(assets_dir);
	return path;
}

std::filesystem::path WorkingDirectory::GetShaderDirectory()
{
	auto path = std::filesystem::current_path();
	path.append(shaders_dir);
	return path;
}

std::filesystem::path WorkingDirectory::EvaluateStandardRootDirectory()
{
	int dir_depth = 0;
	constexpr auto max_depth = 5; //Explicitly limit number of CDs to avoid problems if not found
	bool found = false;

	for (; dir_depth < max_depth; ++dir_depth)
	{
		if (std::filesystem::exists(bin_dir))
		{
			break;
		}

		std::filesystem::path current_path = std::filesystem::current_path();

		for (const auto& entry : std::filesystem::directory_iterator(current_path))
		{
			if (std::filesystem::is_directory(entry))
			{
				std::filesystem::path subDirPath = entry.path();
				std::filesystem::path targetFolderPath = subDirPath / bin_dir;

				if (std::filesystem::exists(targetFolderPath))
				{
					std::filesystem::current_path(subDirPath);
					found = true;
					break;
				}
			}
		}

		if (found)
			break;

		std::filesystem::current_path(changedir_back);
	}

	if (dir_depth == max_depth)
	{
		constexpr auto err_msg = "Root application directory not found";
		Log::Write(Log::Levels::Error, Log::Categories::EngineCore, err_msg);
		throw std::runtime_error(err_msg);
	}
		
	const auto current_path = std::filesystem::current_path();
	const auto msg = "Root application directory set - " + current_path.string();
	Log::Write(Log::Levels::Info, Log::Categories::EngineCore, msg);
	return current_path;
}
