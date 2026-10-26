#pragma once

#include <filesystem>

#include "Macros.h"

namespace Daedalus
{
	class DAEDALUS_API PathConfig
	{
	public:
		PathConfig();
		~PathConfig();
		PathConfig(const PathConfig&) = delete;
		PathConfig& operator=(const PathConfig&) = delete;
		PathConfig(PathConfig&&) = delete;
		PathConfig& operator=(PathConfig&&) = delete;

		static void Load(const std::filesystem::path& path) { GetInstance().LoadImpl(path); }
		static void Save(const std::filesystem::path& path) { GetInstance().SaveImpl(path); }
		static void LoadDefault() { GetInstance().LoadDefaultImpl(); }
		
		static std::filesystem::path GetAssetsPath() { return GetInstance().m_assets_dir; }
		static std::filesystem::path GetShadersPath() { return GetInstance().m_shaders_dir; }
		static std::filesystem::path GetConfigPath() { return GetInstance().m_config_dir; }
		static std::filesystem::path GetLogsPath() { return s_instance ? s_instance->m_logs_dir : std::filesystem::path("logs"); }
		static std::filesystem::path GetScenePath() { return GetInstance().m_assets_dir / GetInstance().m_scene_file; }

		static std::filesystem::path GetGraphicsConfigPath() { return GetInstance().m_config_dir / GetInstance().m_graphics_config_name; }
		static std::filesystem::path GetKeybindConfigPath() { return GetInstance().m_config_dir / GetInstance().m_keybind_config_name; }
				
	private:
		void LoadWorkDirectory(const std::filesystem::path& path);
		void LoadImpl(const std::filesystem::path& path);
		void SaveImpl(const std::filesystem::path& path);
		void LoadDefaultImpl();

		static PathConfig& GetInstance();

		std::filesystem::path m_assets_dir;
		std::filesystem::path m_shaders_dir;
		std::filesystem::path m_logs_dir;
		std::filesystem::path m_scene_file;

		std::filesystem::path m_config_dir;
		std::filesystem::path m_graphics_config_name;
		std::filesystem::path m_keybind_config_name;

		static PathConfig* s_instance;
	};
}