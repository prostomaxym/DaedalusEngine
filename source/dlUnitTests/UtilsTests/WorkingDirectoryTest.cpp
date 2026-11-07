#include <Utils/WorkingDirectory.h>
#include <Platform/Platform.h>
#include <Config/PathConfig.h>
#include <Config/GraphicsConfig.h>
#include <Config/KeybindConfig.h>

#include <gtest/gtest.h>

#include <fstream>
#include <iterator>

namespace Daedalus
{
	namespace
	{
		class WorkingDirectoryTest : public ::testing::Test
		{
		protected:
			void SetUp() override
			{
				m_original_cwd = std::filesystem::current_path();
				m_previous = { WorkingDirectory::GetRootDirectory(), WorkingDirectory::GetPathConfigPath() };
				if (m_previous.runtime_root.empty()) m_previous = { m_original_cwd, m_original_cwd / "path.ini" };
				m_directory = std::filesystem::path(DL_ASSET_TEST_OUTPUT) / "startup_working_directory";
				std::filesystem::remove_all(m_directory);
				m_root = m_directory / "runtime";
				m_settings = m_directory / "settings";
				m_other = m_directory / "other";
				std::filesystem::create_directories(m_root / "config");
				std::filesystem::create_directories(m_settings / "config");
				std::filesystem::create_directories(m_other / "lure/bin");
				ASSERT_TRUE(WorkingDirectory::SetRootDirectory(m_root, m_error)) << m_error;
			}

			void TearDown() override
			{
				EXPECT_TRUE(WorkingDirectory::SetStartupPaths(m_previous, m_error)) << m_error;
				std::filesystem::current_path(m_original_cwd);
				PathConfig::LoadDefault();
				GraphicsConfig::LoadDefault();
				std::filesystem::remove_all(m_directory);
			}

			void WriteFile(const std::filesystem::path& path, const std::string& text)
			{
				std::ofstream file(path);
				file << text;
			}

			std::string ReadFile(const std::filesystem::path& path)
			{
				std::ifstream file(path);
				return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
			}

			void ExpectRejected(const StartupPaths& paths)
			{
				const auto root = WorkingDirectory::GetRootDirectory();
				const auto config = WorkingDirectory::GetPathConfigPath();
				const auto cwd = std::filesystem::current_path();
				EXPECT_FALSE(WorkingDirectory::SetStartupPaths(paths, m_error));
				EXPECT_FALSE(m_error.empty());
				EXPECT_EQ(WorkingDirectory::GetRootDirectory(), root);
				EXPECT_EQ(WorkingDirectory::GetPathConfigPath(), config);
				EXPECT_EQ(std::filesystem::current_path(), cwd);
			}

			StartupPaths m_previous;
			std::filesystem::path m_original_cwd;
			std::filesystem::path m_directory;
			std::filesystem::path m_root;
			std::filesystem::path m_settings;
			std::filesystem::path m_other;
			std::string m_error;
		};
	}

	TEST_F(WorkingDirectoryTest, selected_paths_are_independent_of_later_cwd_changes)
	{
		std::filesystem::current_path(m_other);
		EXPECT_EQ(WorkingDirectory::GetRootDirectory(), m_root);
		EXPECT_EQ(WorkingDirectory::GetPathConfigPath(), m_root / "path.ini");
	}

	TEST_F(WorkingDirectoryTest, missing_selected_config_is_allowed_without_provisioning)
	{
		const auto config = m_settings / "missing.ini";
		ASSERT_TRUE(WorkingDirectory::SetStartupPaths({ m_root, config }, m_error)) << m_error;
		EXPECT_EQ(WorkingDirectory::GetPathConfigPath(), config);
		EXPECT_FALSE(std::filesystem::exists(config));
		EXPECT_FALSE(std::filesystem::exists(m_settings / "asset"));
		EXPECT_EQ(std::filesystem::current_path(), m_root);
	}

	TEST_F(WorkingDirectoryTest, missing_config_defaults_are_anchored_to_selected_config_parent)
	{
		ASSERT_TRUE(WorkingDirectory::SetStartupPaths({ m_root, m_settings / "missing.ini" }, m_error)) << m_error;
		std::filesystem::current_path(m_other);
		WorkingDirectory::LoadConfigs();
		EXPECT_EQ(PathConfig::GetAssetsPath(), m_settings / "asset");
		EXPECT_EQ(PathConfig::GetShadersPath(), m_settings / "shader");
		EXPECT_EQ(PathConfig::GetScriptsPath(), m_settings / "script");
		EXPECT_EQ(PathConfig::GetConfigPath(), m_settings / "config");
		EXPECT_EQ(PathConfig::GetLogsPath(), m_settings / "logs");
		EXPECT_EQ(WorkingDirectory::GetRootDirectory(), m_root);
	}

	TEST_F(WorkingDirectoryTest, load_and_save_use_selected_config_after_cwd_change)
	{
		const auto selected = m_settings / "selected.ini";
		const std::string decoy = "[Path]\nAssetsFolder=must_not_change\n";
		WriteFile(selected, "[Path]\nAssetsFolder=chosen_assets\nScriptsFolder=chosen_scripts\n");
		WriteFile(m_other / "path.ini", decoy);
		ASSERT_TRUE(WorkingDirectory::SetStartupPaths({ m_root, selected }, m_error)) << m_error;
		std::filesystem::current_path(m_other);
		WorkingDirectory::LoadConfigs();
		EXPECT_EQ(PathConfig::GetAssetsPath(), m_settings / "chosen_assets");
		EXPECT_EQ(PathConfig::GetScriptsPath(), m_settings / "chosen_scripts");
		WorkingDirectory::SaveConfigs();
		EXPECT_NE(ReadFile(selected).find("chosen_assets"), std::string::npos);
		EXPECT_EQ(ReadFile(m_other / "path.ini"), decoy);
		EXPECT_FALSE(std::filesystem::exists(m_root / "path.ini"));
		EXPECT_TRUE(std::filesystem::exists(m_settings / "config/graphics.ini"));
		EXPECT_TRUE(std::filesystem::exists(m_settings / "config/keybinds.ini"));
	}

	TEST_F(WorkingDirectoryTest, missing_root_rejection_preserves_selection_and_cwd)
	{
		ExpectRejected({ m_directory / "missing", m_settings / "selected.ini" });
	}

	TEST_F(WorkingDirectoryTest, file_root_rejection_preserves_selection_and_cwd)
	{
		const auto file = m_directory / "not_a_directory";
		WriteFile(file, "file");
		ExpectRejected({ file, m_settings / "selected.ini" });
	}

	TEST_F(WorkingDirectoryTest, missing_config_parent_rejection_preserves_selection_and_cwd)
	{
		ExpectRejected({ m_other, m_directory / "missing/selected.ini" });
	}

	TEST_F(WorkingDirectoryTest, directory_config_rejection_preserves_selection_and_cwd)
	{
		ExpectRejected({ m_other, m_settings / "config" });
	}

	TEST_F(WorkingDirectoryTest, relative_selection_rejection_preserves_selection_and_cwd)
	{
		ExpectRejected({ "relative-root", m_settings / "selected.ini" });
		ExpectRejected({ m_other, "relative.ini" });
	}

	TEST_F(WorkingDirectoryTest, empty_root_is_rejected_without_discovery)
	{
		std::filesystem::current_path(m_other);
		EXPECT_FALSE(WorkingDirectory::SetRootDirectory({}, m_error));
		EXPECT_FALSE(m_error.empty());
		EXPECT_EQ(WorkingDirectory::GetRootDirectory(), m_root);
		EXPECT_EQ(std::filesystem::current_path(), m_other);
	}

	TEST_F(WorkingDirectoryTest, explicit_relative_root_uses_the_callers_directory)
	{
		std::filesystem::current_path(m_directory);
		ASSERT_TRUE(WorkingDirectory::SetRootDirectory("runtime", m_error)) << m_error;
		EXPECT_EQ(WorkingDirectory::GetRootDirectory(), m_root);
		EXPECT_EQ(std::filesystem::current_path(), m_root);
	}

	TEST_F(WorkingDirectoryTest, native_executable_lookup_is_independent_of_cwd)
	{
		std::filesystem::path before;
		ASSERT_TRUE(Platform::GetExecutablePath(before, m_error)) << m_error;
		std::filesystem::current_path(m_other);
		std::filesystem::path after;
		ASSERT_TRUE(Platform::GetExecutablePath(after, m_error)) << m_error;
		EXPECT_EQ(after, before);
		EXPECT_TRUE(after.is_absolute());
		EXPECT_TRUE(std::filesystem::is_regular_file(after));
		EXPECT_TRUE(m_error.empty());
	}

	TEST_F(WorkingDirectoryTest, unicode_selected_root_and_config_paths_are_preserved)
	{
		const auto root = m_directory / std::filesystem::path(u8"runtime 空間");
		std::filesystem::create_directories(root);
		const auto config = root / std::filesystem::path(u8"selected 空間.ini");
		ASSERT_TRUE(WorkingDirectory::SetStartupPaths({ root, config }, m_error)) << m_error;
		PathConfig::Load(WorkingDirectory::GetPathConfigPath());
		EXPECT_EQ(PathConfig::GetAssetsPath(), root / "asset");
		EXPECT_EQ(PathConfig::GetScriptsPath(), root / "script");
		PathConfig::Save(WorkingDirectory::GetPathConfigPath());
		EXPECT_TRUE(std::filesystem::is_regular_file(config));
	}
}
