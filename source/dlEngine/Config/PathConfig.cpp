#include "dlpch.h"

#include "PathConfig.h"
#include <SimpleIni.h>

using namespace Daedalus;

namespace
{
	constexpr auto PathSectionName = "Path";
	constexpr auto AssetsDirName = "AssetsFolder";
	constexpr auto ShadersDirName = "ShadersFolder";
	constexpr auto LogsDirName = "LogsFolder";
	
	constexpr auto ConfigSectionName = "Config";
	constexpr auto ConfigDirName = "ConfigsFolder";
	constexpr auto GraphicConfigName = "GraphicsConfigFile";
	constexpr auto KeybindConfigName = "KeybindConfigFile";
}

std::filesystem::path PathConfig::s_assets_dir = "asset";
std::filesystem::path PathConfig::s_shaders_dir = "shader";
std::filesystem::path PathConfig::s_config_dir = "config";
std::filesystem::path PathConfig::s_logs_dir = "logs";
std::filesystem::path PathConfig::s_graphics_config_name = "graphics.ini";
std::filesystem::path PathConfig::s_keybind_config_name = "keybinds.ini";
		
void PathConfig::Load(const std::filesystem::path& path)
{
	LoadWorkDirectory(path.parent_path());

	CSimpleIniA ini;
    ini.SetUnicode();

    SI_Error rc = ini.LoadFile(path.string().c_str());
    if (rc < 0)
	{
		Log::Write(Log::Levels::Warn, Log::Categories::Application, "Failed to load path config from " + path.string() + "\nLoading default config");
        LoadDefault();
		return;
    }
	else
	{
		Log::Write(Log::Levels::Info, Log::Categories::Application, "Loaded path config from " + path.string());
	}

	s_assets_dir /= ini.GetValue(PathSectionName, AssetsDirName, "asset");
	s_shaders_dir /= ini.GetValue(PathSectionName, ShadersDirName, "shader");
	s_config_dir /= ini.GetValue(ConfigSectionName, ConfigDirName, "config");
	s_logs_dir /= ini.GetValue(PathSectionName, LogsDirName, "logs");
	s_graphics_config_name = ini.GetValue(ConfigSectionName, GraphicConfigName, "graphics.ini");
	s_keybind_config_name = ini.GetValue(ConfigSectionName, KeybindConfigName, "keybinds.ini");
}

void PathConfig::Save(const std::filesystem::path& path)
{
	CSimpleIniA ini;
    ini.SetUnicode();

	ini.SetValue(PathSectionName, AssetsDirName, s_assets_dir.string().c_str());
	ini.SetValue(PathSectionName, ShadersDirName, s_shaders_dir.string().c_str());
	ini.SetValue(PathSectionName, LogsDirName, s_logs_dir.string().c_str());
	ini.SetValue(ConfigSectionName, ConfigDirName, s_config_dir.string().c_str());
	ini.SetValue(ConfigSectionName, GraphicConfigName, s_graphics_config_name.string().c_str());
	ini.SetValue(ConfigSectionName, KeybindConfigName, s_keybind_config_name.string().c_str());
	
    SI_Error rc = ini.SaveFile(path.string().c_str());
    if (rc < 0) 
	{
        Log::Write(Log::Levels::Warn, Log::Categories::Application, "Failed to save path config to " + path.string() + "\n");
    }
	else
	{
		Log::Write(Log::Levels::Info, Log::Categories::Application, "Saved path config to " + path.string());
	}
}

void PathConfig::LoadDefault()
{
	s_assets_dir = std::filesystem::current_path() / "asset";
	s_shaders_dir = std::filesystem::current_path() / "shader";
	s_config_dir = std::filesystem::current_path() / "config";
	s_logs_dir = std::filesystem::current_path() / "logs";
	s_graphics_config_name = "graphics.ini";
	s_keybind_config_name = "keybinds.ini";
}

void PathConfig::LoadWorkDirectory(const std::filesystem::path& path)
{
	s_assets_dir = path;
	s_shaders_dir = path;
	s_config_dir = path;
	s_logs_dir = path;
}