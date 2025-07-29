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
		
		static std::filesystem::path GetAssetsPath() { return s_assets_subdir; }
		static std::filesystem::path GetShadersPath() { return s_shaders_subdir; }
		static std::filesystem::path GetGraphicsConfigPath() { return s_config_subdir / s_graphics_config; }
		static std::filesystem::path GetKeybindConfigPath() { return s_config_subdir / s_keybind_config; }
		static std::filesystem::path GetConfigPath() { return s_config_subdir; }
		
	private:
		static std::filesystem::path s_assets_subdir;
		static std::filesystem::path s_shaders_subdir;
		
		static std::filesystem::path s_config_subdir;
		static std::filesystem::path s_graphics_config;
		static std::filesystem::path s_keybind_config;
	};
}