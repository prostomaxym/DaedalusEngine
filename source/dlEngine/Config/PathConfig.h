#pragma once

#include <filesystem>

#include "Macros.h"

namespace Daedalus
{
	class DAEDALUS_API PathConfig
	{
	public:
		static void Load(const std::filesystem::path& path);
		static void Save(const std::filesystem::path& path);
		static void LoadDefault();
		
		static std::filesystem::path GetAssetsPath() { return s_assets_dir; }
		static std::filesystem::path GetShadersPath() { return s_shaders_dir; }
		static std::filesystem::path GetConfigPath() { return s_config_dir; }
		static std::filesystem::path GetLogsPath() { return s_logs_dir; }

		static std::filesystem::path GetGraphicsConfigPath() { return s_config_dir / s_graphics_config_name; }
		static std::filesystem::path GetKeybindConfigPath() { return s_config_dir / s_keybind_config_name; }
				
	private:
		static void LoadWorkDirectory(const std::filesystem::path& path);

		static std::filesystem::path s_assets_dir;
		static std::filesystem::path s_shaders_dir;
		static std::filesystem::path s_logs_dir;

		static std::filesystem::path s_config_dir;
		static std::filesystem::path s_graphics_config_name;
		static std::filesystem::path s_keybind_config_name;
	};
}