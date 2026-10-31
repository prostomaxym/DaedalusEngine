#include <Config/PathConfig.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

using Daedalus::PathConfig;

namespace
{
	class PathConfigTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			m_directory = std::filesystem::temp_directory_path() / "dl_path_config_tests";
			std::filesystem::create_directories(m_directory);
			m_config_path = m_directory / "path.ini";
			PathConfig::LoadDefault();
		}

		void TearDown() override
		{
			PathConfig::LoadDefault();
			std::filesystem::remove_all(m_directory);
		}

		void WriteConfig(const std::string& text)
		{
			std::ofstream file(m_config_path);
			file << text;
		}

		std::string ReadSetting(const std::filesystem::path& path, const std::string& key)
		{
			const auto trim = [](const std::string& value)
			{
				const auto first = value.find_first_not_of(" \t\r");
				return first == std::string::npos ? std::string() :
					value.substr(first, value.find_last_not_of(" \t\r") - first + 1);
			};
			std::ifstream file(path);
			std::string line;
			while (std::getline(file, line))
			{
				const auto separator = line.find('=');
				if (separator != std::string::npos && trim(line.substr(0, separator)) == key)
					return trim(line.substr(separator + 1));
			}
			return {};
		}

		std::filesystem::path m_directory;
		std::filesystem::path m_config_path;
	};
}

TEST_F(PathConfigTest, default_scene_is_relative_to_default_assets)
{
	EXPECT_EQ(PathConfig::GetScenePath(),
		std::filesystem::current_path() / "asset/scenes/startup.scene.json");
}

TEST_F(PathConfigTest, configured_scene_is_relative_to_configured_assets)
{
	WriteConfig("[Path]\nAssetsFolder=game_assets\nSceneFile=levels/first.scene.json\n");

	PathConfig::Load(m_config_path);

	EXPECT_EQ(PathConfig::GetAssetsPath(), m_directory / "game_assets");
	EXPECT_EQ(PathConfig::GetScenePath(), m_directory / "game_assets/levels/first.scene.json");
}

TEST_F(PathConfigTest, absolute_asset_root_is_preserved)
{
	const auto assets = m_directory / "external_assets";
	WriteConfig("[Path]\nAssetsFolder=" + assets.generic_string() + "\nSceneFile=levels/game.scene.json\n");

	PathConfig::Load(m_config_path);

	EXPECT_EQ(PathConfig::GetScenePath(), assets / "levels/game.scene.json");
}

TEST_F(PathConfigTest, repeated_load_replaces_scene_and_asset_paths)
{
	WriteConfig("[Path]\nAssetsFolder=first_assets\nSceneFile=first.scene.json\n");
	PathConfig::Load(m_config_path);
	WriteConfig("[Path]\nAssetsFolder=second_assets\nSceneFile=second.scene.json\n");

	PathConfig::Load(m_config_path);

	EXPECT_EQ(PathConfig::GetScenePath(), m_directory / "second_assets/second.scene.json");
}

TEST_F(PathConfigTest, missing_scene_setting_restores_generic_default)
{
	WriteConfig("[Path]\nSceneFile=custom.scene.json\n");
	PathConfig::Load(m_config_path);
	WriteConfig("[Path]\nAssetsFolder=game_assets\n");

	PathConfig::Load(m_config_path);

	EXPECT_EQ(PathConfig::GetScenePath(), m_directory / "game_assets/scenes/startup.scene.json");
}

TEST_F(PathConfigTest, missing_config_restores_default_scene)
{
	WriteConfig("[Path]\nSceneFile=custom.scene.json\n");
	PathConfig::Load(m_config_path);
	std::filesystem::remove(m_config_path);

	PathConfig::Load(m_config_path);

	EXPECT_EQ(PathConfig::GetScenePath(),
		m_directory / "asset/scenes/startup.scene.json");
	EXPECT_EQ(PathConfig::GetShadersPath(), m_directory / "shader");
	EXPECT_EQ(PathConfig::GetConfigPath(), m_directory / "config");
	EXPECT_EQ(PathConfig::GetLogsPath(), m_directory / "logs");
}

TEST_F(PathConfigTest, save_then_load_preserves_scene_selection)
{
	WriteConfig("[Path]\nAssetsFolder=game_assets\nSceneFile=levels/second.scene.json\n");
	PathConfig::Load(m_config_path);
	const auto expected_scene = PathConfig::GetScenePath();
	const auto saved_path = m_directory / "saved.ini";
	PathConfig::Save(saved_path);
	PathConfig::LoadDefault();

	PathConfig::Load(saved_path);

	EXPECT_EQ(PathConfig::GetScenePath(), expected_scene);
}

TEST_F(PathConfigTest, relative_ini_filename_resolves_all_folders_against_ini_directory)
{
	WriteConfig("[Path]\nAssetsFolder=assets\nShadersFolder=shaders\nLogsFolder=log\n"
		"[Config]\nConfigsFolder=settings\nGraphicsConfigFile=video.ini\nKeybindConfigFile=input.ini\n");

	const auto previous_working_directory = std::filesystem::current_path();
	std::filesystem::current_path(m_directory.parent_path());
	PathConfig::Load(m_directory.filename() / m_config_path.filename());
	std::filesystem::current_path(previous_working_directory);

	EXPECT_EQ(PathConfig::GetAssetsPath(), m_directory / "assets");
	EXPECT_EQ(PathConfig::GetShadersPath(), m_directory / "shaders");
	EXPECT_EQ(PathConfig::GetLogsPath(), m_directory / "log");
	EXPECT_EQ(PathConfig::GetConfigPath(), m_directory / "settings");
	EXPECT_EQ(PathConfig::GetGraphicsConfigPath(), m_directory / "settings/video.ini");
	EXPECT_EQ(PathConfig::GetKeybindConfigPath(), m_directory / "settings/input.ini");
}

TEST_F(PathConfigTest, save_writes_relative_paths_for_all_entries)
{
	WriteConfig("[Path]\nAssetsFolder=game_assets\nShadersFolder=shaders\nLogsFolder=log\n"
		"SceneFile=levels/game.scene.json\n[Config]\nConfigsFolder=settings\n"
		"GraphicsConfigFile=video.ini\nKeybindConfigFile=input.ini\n");
	PathConfig::Load(m_config_path);

	PathConfig::Save(m_config_path);

	EXPECT_EQ(ReadSetting(m_config_path, "AssetsFolder"), "game_assets");
	EXPECT_EQ(ReadSetting(m_config_path, "ShadersFolder"), "shaders");
	EXPECT_EQ(ReadSetting(m_config_path, "LogsFolder"), "log");
	EXPECT_EQ(ReadSetting(m_config_path, "ConfigsFolder"), "settings");
	EXPECT_EQ(ReadSetting(m_config_path, "SceneFile"), "levels/game.scene.json");
	EXPECT_EQ(ReadSetting(m_config_path, "GraphicsConfigFile"), "video.ini");
	EXPECT_EQ(ReadSetting(m_config_path, "KeybindConfigFile"), "input.ini");
}

TEST_F(PathConfigTest, save_converts_legacy_absolute_entries_to_relative_paths)
{
	WriteConfig("[Path]\nAssetsFolder=" + (m_directory / "game_assets").generic_string() +
		"\nSceneFile=" + (m_directory / "game_assets/levels/game.scene.json").generic_string() +
		"\nShadersFolder=" + (m_directory / "shaders").generic_string() +
		"\nLogsFolder=" + (m_directory / "log").generic_string() +
		"\n[Config]\nConfigsFolder=" + (m_directory / "settings").generic_string() +
		"\nGraphicsConfigFile=" + (m_directory / "settings/video.ini").generic_string() +
		"\nKeybindConfigFile=" + (m_directory / "settings/input.ini").generic_string() + "\n");
	PathConfig::Load(m_config_path);
	const auto expected_scene = PathConfig::GetScenePath();

	PathConfig::Save(m_config_path);

	EXPECT_EQ(ReadSetting(m_config_path, "AssetsFolder"), "game_assets");
	EXPECT_EQ(ReadSetting(m_config_path, "ShadersFolder"), "shaders");
	EXPECT_EQ(ReadSetting(m_config_path, "LogsFolder"), "log");
	EXPECT_EQ(ReadSetting(m_config_path, "ConfigsFolder"), "settings");
	EXPECT_EQ(ReadSetting(m_config_path, "SceneFile"), "levels/game.scene.json");
	EXPECT_EQ(ReadSetting(m_config_path, "GraphicsConfigFile"), "video.ini");
	EXPECT_EQ(ReadSetting(m_config_path, "KeybindConfigFile"), "input.ini");
	PathConfig::Load(m_config_path);
	EXPECT_EQ(PathConfig::GetScenePath(), expected_scene);
}

TEST_F(PathConfigTest, save_in_another_directory_preserves_resolved_paths)
{
	WriteConfig("[Path]\nAssetsFolder=game_assets\n");
	PathConfig::Load(m_config_path);
	const auto saved_path = m_directory / "nested/path.ini";
	std::filesystem::create_directories(saved_path.parent_path());

	PathConfig::Save(saved_path);

	EXPECT_EQ(ReadSetting(saved_path, "AssetsFolder"), "../game_assets");
	PathConfig::Load(saved_path);
	EXPECT_EQ(PathConfig::GetAssetsPath(), m_directory / "game_assets");
	EXPECT_EQ(PathConfig::GetShadersPath(), m_directory / "shader");
	EXPECT_EQ(PathConfig::GetConfigPath(), m_directory / "config");
	EXPECT_EQ(PathConfig::GetLogsPath(), m_directory / "logs");
}

TEST_F(PathConfigTest, saved_config_can_be_relocated_with_runtime_folders)
{
	WriteConfig("[Path]\nAssetsFolder=game_assets\nSceneFile=levels/game.scene.json\n");
	PathConfig::Load(m_config_path);
	PathConfig::Save(m_config_path);
	const auto relocated_path = m_directory / "relocated/path.ini";
	std::filesystem::create_directories(relocated_path.parent_path());
	std::filesystem::copy_file(m_config_path, relocated_path);

	PathConfig::Load(relocated_path);

	EXPECT_EQ(PathConfig::GetScenePath(), relocated_path.parent_path() / "game_assets/levels/game.scene.json");
	EXPECT_EQ(PathConfig::GetShadersPath(), relocated_path.parent_path() / "shader");
	EXPECT_EQ(PathConfig::GetConfigPath(), relocated_path.parent_path() / "config");
	EXPECT_EQ(PathConfig::GetLogsPath(), relocated_path.parent_path() / "logs");
}
