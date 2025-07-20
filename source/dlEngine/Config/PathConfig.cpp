#include "dlpch.h"

#include "PathConfig.h"
#include <SimpleIni.h>

using namespace Daedalus;

namespace
{
	constexpr auto PathSectionName = "Path";
	constexpr auto AssetsDirName = "AssetsFolder";
	constexpr auto ShadersDirName = "ShadersFolder";
	
	constexpr auto ConfigSectionName = "Config";
	constexpr auto ConfigDirName = "ConfigsFolder";
	constexpr auto GraphicConfigName = "GraphicsConfigFile";
	constexpr auto KeybindConfigName = "KeybindConfigFile";
}

std::filesystem::path PathConfig::s_assets_subdir = "asset";
std::filesystem::path PathConfig::s_shaders_subdir = "shader";
std::filesystem::path PathConfig::s_config_subdir = "config";
std::filesystem::path PathConfig::s_graphics_config = "graphics.ini";
std::filesystem::path PathConfig::s_keybind_config = "keybinds.ini";
		
void PathConfig::Load(const std::filesystem::path& path)
{
	CSimpleIniA ini;
    ini.SetUnicode();

    SI_Error rc = ini.LoadFile(path.string().c_str());
    if (rc < 0)
	{
		Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "Failed to load path config from " + path.string() + "\nLoading default config");
        LoadDefault();
		return;
    }
	else
	{
		Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Loaded path config from " + path.string());
	}

	s_assets_subdir = ini.GetValue(PathSectionName, AssetsDirName, "asset");
	s_shaders_subdir = ini.GetValue(PathSectionName, ShadersDirName, "shader");
	s_config_subdir = ini.GetValue(ConfigSectionName, ConfigDirName, "config");
	s_graphics_config = ini.GetValue(ConfigSectionName, GraphicConfigName, "graphics.ini");
	s_keybind_config = ini.GetValue(ConfigSectionName, KeybindConfigName, "keybinds.ini");
}

void PathConfig::Save(const std::filesystem::path& path)
{
	CSimpleIniA ini;
    ini.SetUnicode();

	ini.SetValue(PathSectionName, AssetsDirName, s_assets_subdir.string().c_str());
	ini.SetValue(PathSectionName, ShadersDirName, s_shaders_subdir.string().c_str());
	ini.SetValue(ConfigSectionName, ConfigDirName, s_config_subdir.string().c_str());
	ini.SetValue(ConfigSectionName, GraphicConfigName, s_graphics_config.string().c_str());
	ini.SetValue(ConfigSectionName, KeybindConfigName, s_keybind_config.string().c_str());
	
    SI_Error rc = ini.SaveFile(path.string().c_str());
    if (rc < 0) 
	{
        Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "Failed to save path config to " + path.string() + "\n");
    }
	else
	{
		Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Saved path config to " + path.string());
	}
}

void PathConfig::LoadDefault()
{
	s_assets_subdir = "asset";
	s_shaders_subdir = "shader";
	s_config_subdir = "config";
	s_graphics_config = "graphics.ini";
	s_keybind_config = "keybinds.ini";
}