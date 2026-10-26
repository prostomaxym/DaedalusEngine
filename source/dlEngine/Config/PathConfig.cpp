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
	constexpr auto SceneFileName = "SceneFile";
	constexpr auto DefaultSceneFile = "scenes/startup.scene.json";
	
	constexpr auto ConfigSectionName = "Config";
	constexpr auto ConfigDirName = "ConfigsFolder";
	constexpr auto GraphicConfigName = "GraphicsConfigFile";
	constexpr auto KeybindConfigName = "KeybindConfigFile";
}

PathConfig* PathConfig::s_instance = nullptr;

PathConfig::PathConfig()
	: m_assets_dir("asset")
	, m_shaders_dir("shader")
	, m_config_dir("config")
	, m_logs_dir("logs")
	, m_scene_file(DefaultSceneFile)
	, m_graphics_config_name("graphics.ini")
	, m_keybind_config_name("keybinds.ini")
{
	DL_ASSERT(!s_instance, Log::Categories::Application, "Only one path configuration may be active");
	if (s_instance) std::abort();
	s_instance = this;
}

PathConfig::~PathConfig()
{
	s_instance = nullptr;
}

PathConfig& PathConfig::GetInstance()
{
	DL_ASSERT(s_instance, Log::Categories::Application, "PathConfig used outside its lifetime");
	if (!s_instance) std::abort();
	return *s_instance;
}

void PathConfig::LoadImpl(const std::filesystem::path& path)
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

	m_assets_dir /= ini.GetValue(PathSectionName, AssetsDirName, "asset");
	m_shaders_dir /= ini.GetValue(PathSectionName, ShadersDirName, "shader");
	m_config_dir /= ini.GetValue(ConfigSectionName, ConfigDirName, "config");
	m_logs_dir /= ini.GetValue(PathSectionName, LogsDirName, "logs");
	m_scene_file = ini.GetValue(PathSectionName, SceneFileName, DefaultSceneFile);
	m_graphics_config_name = ini.GetValue(ConfigSectionName, GraphicConfigName, "graphics.ini");
	m_keybind_config_name = ini.GetValue(ConfigSectionName, KeybindConfigName, "keybinds.ini");
}

void PathConfig::SaveImpl(const std::filesystem::path& path)
{
	CSimpleIniA ini;
    ini.SetUnicode();

	ini.SetValue(PathSectionName, AssetsDirName, m_assets_dir.string().c_str());
	ini.SetValue(PathSectionName, ShadersDirName, m_shaders_dir.string().c_str());
	ini.SetValue(PathSectionName, LogsDirName, m_logs_dir.string().c_str());
	ini.SetValue(PathSectionName, SceneFileName, m_scene_file.string().c_str());
	ini.SetValue(ConfigSectionName, ConfigDirName, m_config_dir.string().c_str());
	ini.SetValue(ConfigSectionName, GraphicConfigName, m_graphics_config_name.string().c_str());
	ini.SetValue(ConfigSectionName, KeybindConfigName, m_keybind_config_name.string().c_str());
	
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

void PathConfig::LoadDefaultImpl()
{
	m_assets_dir = std::filesystem::current_path() / "asset";
	m_shaders_dir = std::filesystem::current_path() / "shader";
	m_config_dir = std::filesystem::current_path() / "config";
	m_logs_dir = std::filesystem::current_path() / "logs";
	m_scene_file = DefaultSceneFile;
	m_graphics_config_name = "graphics.ini";
	m_keybind_config_name = "keybinds.ini";
}

void PathConfig::LoadWorkDirectory(const std::filesystem::path& path)
{
	m_assets_dir = path;
	m_shaders_dir = path;
	m_config_dir = path;
	m_logs_dir = path;
}