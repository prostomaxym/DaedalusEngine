#include "StartupPaths.h"
#include "Utf8.h"

#include <utility>

namespace Daedalus
{
	namespace
	{
		std::filesystem::path Normalize(const std::filesystem::path& path)
		{
			auto normalized = path.lexically_normal();
			if (!normalized.has_filename() && normalized != normalized.root_path())
				normalized = normalized.parent_path();
			return normalized;
		}

		bool IsPlatformDirectory(const std::filesystem::path& name)
		{
			return name == "windows-x64" || name == "linux-x64";
		}

		std::filesystem::path DefaultRoot(const std::filesystem::path& executable_path)
		{
			auto directory = executable_path.parent_path();
			const auto configuration = directory.filename();
			if ((configuration == "Debug" || configuration == "Release" || configuration == "ReleaseDevTools")
				&& IsPlatformDirectory(directory.parent_path().filename())
				&& directory.parent_path().parent_path().filename() == "bin")
				directory = directory.parent_path();
			if (IsPlatformDirectory(directory.filename()) && directory.parent_path().filename() == "bin")
				directory = directory.parent_path();
			return directory.filename() == "bin" ? directory.parent_path() : directory;
		}
	}

	bool StartupDiscovery::Resolve(const std::filesystem::path& executable_path,
		const std::filesystem::path& launch_directory,
		std::span<const std::filesystem::path> arguments, StartupPaths& result, std::string& error)
	{
		if (!executable_path.is_absolute() || !executable_path.has_filename() || !launch_directory.is_absolute())
		{
			error = "Startup selection requires an absolute executable path and launch directory";
			return false;
		}

		std::filesystem::path root_override;
		std::filesystem::path config_override;
		for (std::size_t index = 0; index < arguments.size(); ++index)
		{
			const auto& option = arguments[index];
			std::filesystem::path* destination = nullptr;
			if (option == "--runtime-root") destination = &root_override;
			else if (option == "--path-config") destination = &config_override;
			else
			{
				error = "Unknown startup option: " + Utf8ToString(option.generic_u8string());
				return false;
			}
			if (!destination->empty())
			{
				error = "Duplicate startup option: " + Utf8ToString(option.generic_u8string());
				return false;
			}
			if (++index == arguments.size() || arguments[index].empty()
				|| arguments[index].native().find(std::filesystem::path("--").native()) == 0)
			{
				error = "Missing path for startup option: " + Utf8ToString(option.generic_u8string());
				return false;
			}
			const auto& value = arguments[index];
			if (!value.is_absolute() && (value.has_root_name() || value.has_root_directory()))
			{
				error = "Startup paths must be absolute or relative to the launch directory, not drive-relative";
				return false;
			}
			*destination = Normalize(value.is_absolute() ? value : launch_directory / value);
		}

		StartupPaths selected;
		selected.runtime_root = !root_override.empty() ? root_override :
			!config_override.empty() ? config_override.parent_path() : DefaultRoot(Normalize(executable_path));
		selected.path_config = !config_override.empty() ? config_override : selected.runtime_root / "path.ini";
		result = std::move(selected);
		error.clear();
		return true;
	}
}
