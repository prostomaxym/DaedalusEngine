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
		std::filesystem::current_path() / "asset/scenes/startup.scene.json");
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
