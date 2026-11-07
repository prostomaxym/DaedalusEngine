#pragma once

#include "Application.h"
#include "Debug/Log.h"
#include "Platform/Platform.h"
#include "Utils/StartupPaths.h"
#include "Utils/Utf8.h"
#include "Utils/WorkingDirectory.h"

#include <cstdlib>
#include <memory>
#include <vector>

extern Daedalus::Application* Daedalus::CreateApplication();

#ifndef DL_PLATFORM_ANDROID
#ifdef DL_PLATFORM_WINDOWS
int wmain(int argc, wchar_t** argv)
#else
int main(int argc, char** argv)
#endif
{
	Daedalus::Log::Init();
	Daedalus::Log::Write(Daedalus::Log::Levels::Info, Daedalus::Log::Categories::EngineCore, "Initialized core logger;");

	const auto show_usage = []
	{
		Daedalus::Log::Write(Daedalus::Log::Levels::Info, Daedalus::Log::Categories::EngineCore,
			"Usage: application [--runtime-root <directory>] [--path-config <file>]\n"
			"Relative overrides use the launch directory; a config-only override selects its parent as root.\n"
			"Without overrides, fixed executable-relative layouts select the runtime root.\n"
			"A missing path config uses defaults beside that config. Use --help alone to show this message.");
	};
	if (argc == 2 && std::filesystem::path(argv[1]) == "--help")
	{
		show_usage();
		Daedalus::Log::Shutdown();
		return EXIT_SUCCESS;
	}
	std::error_code code;
	const auto launch_directory = std::filesystem::current_path(code);
	std::filesystem::path executable_path;
	std::vector<std::filesystem::path> arguments;
	for (int index = 1; index < argc; ++index) arguments.emplace_back(argv[index]);
	Daedalus::StartupPaths paths;
	std::string error;
	const auto selected = !code && Daedalus::Platform::GetExecutablePath(executable_path, error)
		&& Daedalus::StartupDiscovery::Resolve(executable_path, launch_directory, arguments, paths, error)
		&& Daedalus::WorkingDirectory::SetStartupPaths(paths, error);
	if (!selected)
	{
		// Error-file logging cannot safely select a destination before runtime-path validation.
		Daedalus::Log::Write(Daedalus::Log::Levels::Warn, Daedalus::Log::Categories::EngineCore,
			"Startup failed: {}", code ? code.message() : error);
		show_usage();
		Daedalus::Log::Shutdown();
		return EXIT_FAILURE;
	}
	Daedalus::Log::Write(Daedalus::Log::Levels::Info, Daedalus::Log::Categories::EngineCore,
		"Runtime root: {}; path config: {}", Daedalus::Utf8ToString(paths.runtime_root.generic_u8string()),
		Daedalus::Utf8ToString(paths.path_config.generic_u8string()));
	{
		const std::unique_ptr<Daedalus::Application> app(Daedalus::CreateApplication());
		app->Run();
	}

	Daedalus::Log::Shutdown();

	return EXIT_SUCCESS;
}
#endif
