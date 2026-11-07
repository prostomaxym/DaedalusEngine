#include "WorkingDirectory.h"
#include "Utf8.h"

#include "Config/GraphicsConfig.h"
#include "Config/KeybindConfig.h"
#include "Config/PathConfig.h"
#include "Debug/Log.h"
#include "Debug/ImGuiLayer.h"

#include <cstdlib>
#include <utility>

using namespace Daedalus;

namespace
{
	void RequireStartupPaths(const StartupPaths& paths)
	{
		if (paths.path_config.empty())
		{
			Log::Write(Log::Levels::Error, Log::Categories::EngineCore,
				"Runtime paths must be selected before application construction");
			std::abort();
		}
	}
}

StartupPaths WorkingDirectory::s_startup_paths;

bool WorkingDirectory::SetRootDirectory(const std::filesystem::path& path, std::string& error)
{
	if (path.empty() || (!path.is_absolute() && (path.has_root_name() || path.has_root_directory())))
	{
		error = "An explicit absolute or launch-relative runtime root is required";
		return false;
	}
	std::error_code code;
	const auto root = std::filesystem::absolute(path, code).lexically_normal();
	if (code)
	{
		error = "Cannot resolve the runtime root: " + code.message();
		return false;
	}
	return SetStartupPaths({ root, root / "path.ini" }, error);
}

std::filesystem::path WorkingDirectory::GetRootDirectory()
{
	return s_startup_paths.runtime_root;
}

std::filesystem::path WorkingDirectory::GetPathConfigPath()
{
	return s_startup_paths.path_config;
}

bool WorkingDirectory::SetStartupPaths(const StartupPaths& paths, std::string& error)
{
	if (!paths.runtime_root.is_absolute() || !paths.path_config.is_absolute() || !paths.path_config.has_filename())
	{
		error = "Selected runtime root and path config must be absolute paths";
		return false;
	}
	StartupPaths selected{ paths.runtime_root.lexically_normal(), paths.path_config.lexically_normal() };
	std::error_code code;
	if (!std::filesystem::is_directory(selected.runtime_root, code))
	{
		error = "Runtime root is not an accessible directory: " + Utf8ToString(selected.runtime_root.generic_u8string());
		if (code) error += " (" + code.message() + ")";
		return false;
	}
	if (!std::filesystem::is_directory(selected.path_config.parent_path(), code))
	{
		error = "Path config parent is not an accessible directory: " + Utf8ToString(selected.path_config.parent_path().generic_u8string());
		if (code) error += " (" + code.message() + ")";
		return false;
	}
	const auto config_status = std::filesystem::status(selected.path_config, code);
	if ((code && code != std::errc::no_such_file_or_directory)
		|| (std::filesystem::exists(config_status) && !std::filesystem::is_regular_file(config_status)))
	{
		error = "Path config must be a regular file or a missing file: " + Utf8ToString(selected.path_config.generic_u8string());
		if (code) error += " (" + code.message() + ")";
		return false;
	}
	std::filesystem::current_path(selected.runtime_root, code);
	if (code)
	{
		error = "Cannot select the runtime working directory: " + code.message();
		return false;
	}
	s_startup_paths = std::move(selected);
	error.clear();
	return true;
}

void WorkingDirectory::LoadConfigs()
{
	RequireStartupPaths(s_startup_paths);
	PathConfig::Load(s_startup_paths.path_config);
	GraphicsConfig::Load(PathConfig::GetGraphicsConfigPath());
	KeybindConfig::Load(PathConfig::GetKeybindConfigPath());
	ImGuiLayer::LoadConfig();
}

void WorkingDirectory::SaveConfigs()
{
	RequireStartupPaths(s_startup_paths);
	PathConfig::Save(s_startup_paths.path_config);
	GraphicsConfig::Save(PathConfig::GetGraphicsConfigPath());
	KeybindConfig::Save(PathConfig::GetKeybindConfigPath());
}