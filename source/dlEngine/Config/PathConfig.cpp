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

	std::filesystem::path MakeRelativePath(const std::filesystem::path& path, const std::filesystem::path& base)
	{
		const auto absolute_path = std::filesystem::absolute(path).lexically_normal();
		const auto relative_path = absolute_path.lexically_relative(std::filesystem::absolute(base).lexically_normal());
		if (relative_path.empty())
		{
			Log::Write(Log::Levels::Warn, Log::Categories::Application,
				"Cannot save {} relative to {}; preserving absolute path", absolute_path.string(), base.string());
			return absolute_path;
		}
		return relative_path;
	}
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
	const auto base = std::filesystem::absolute(path).lexically_normal().parent_path();
	LoadDefault();
	LoadWorkDirectory(base);

	CSimpleIniA ini;
    ini.SetUnicode();

    SI_Error rc = ini.LoadFile(path.string().c_str());
    if (rc < 0)
	{
		Log::Write(Log::Levels::Warn, Log::Categories::Application, "Failed to load path config from {}\nLoading default config", path.string());
		return;
    }
	else
	{
		Log::Write(Log::Levels::Info, Log::Categories::Application, "Loaded path config from {}", path.string());
	}

	m_assets_dir = (base / ini.GetValue(PathSectionName, AssetsDirName, "asset")).lexically_normal();
	m_shaders_dir = (base / ini.GetValue(PathSectionName, ShadersDirName, "shader")).lexically_normal();
	m_config_dir = (base / ini.GetValue(ConfigSectionName, ConfigDirName, "config")).lexically_normal();
	m_logs_dir = (base / ini.GetValue(PathSectionName, LogsDirName, "logs")).lexically_normal();
	m_scene_file = ini.GetValue(PathSectionName, SceneFileName, DefaultSceneFile);
	m_graphics_config_name = ini.GetValue(ConfigSectionName, GraphicConfigName, "graphics.ini");
	m_keybind_config_name = ini.GetValue(ConfigSectionName, KeybindConfigName, "keybinds.ini");
}

void PathConfig::SaveImpl(const std::filesystem::path& path)
{
	const auto base = std::filesystem::absolute(path).lexically_normal().parent_path();
	CSimpleIniA ini;
    ini.SetUnicode();

	ini.SetValue(PathSectionName, AssetsDirName, MakeRelativePath(m_assets_dir, base).generic_string().c_str());
	ini.SetValue(PathSectionName, ShadersDirName, MakeRelativePath(m_shaders_dir, base).generic_string().c_str());
	ini.SetValue(PathSectionName, LogsDirName, MakeRelativePath(m_logs_dir, base).generic_string().c_str());
	ini.SetValue(PathSectionName, SceneFileName, MakeRelativePath(m_assets_dir / m_scene_file, m_assets_dir).generic_string().c_str());
	ini.SetValue(ConfigSectionName, ConfigDirName, MakeRelativePath(m_config_dir, base).generic_string().c_str());
	ini.SetValue(ConfigSectionName, GraphicConfigName, MakeRelativePath(m_config_dir / m_graphics_config_name, m_config_dir).generic_string().c_str());
	ini.SetValue(ConfigSectionName, KeybindConfigName, MakeRelativePath(m_config_dir / m_keybind_config_name, m_config_dir).generic_string().c_str());
	
    SI_Error rc = ini.SaveFile(path.string().c_str());
    if (rc < 0) 
	{
		Log::Write(Log::Levels::Warn, Log::Categories::Application, "Failed to save path config to {}\n", path.string());
    }
	else
	{
		Log::Write(Log::Levels::Info, Log::Categories::Application, "Saved path config to {}", path.string());
	}
}

void PathConfig::LoadDefaultImpl()
{
	LoadWorkDirectory(std::filesystem::current_path());
	m_scene_file = DefaultSceneFile;
	m_graphics_config_name = "graphics.ini";
	m_keybind_config_name = "keybinds.ini";
}

void PathConfig::LoadWorkDirectory(const std::filesystem::path& path)
{
	m_assets_dir = path / "asset";
	m_shaders_dir = path / "shader";
	m_config_dir = path / "config";
	m_logs_dir = path / "logs";
}