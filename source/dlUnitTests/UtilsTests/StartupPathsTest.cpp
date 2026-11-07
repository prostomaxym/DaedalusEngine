#include <Utils/StartupPaths.h>

#include <gtest/gtest.h>

#include <vector>

namespace Daedalus
{
	namespace
	{
		class StartupDiscoveryTest : public ::testing::Test
		{
		protected:
			std::filesystem::path m_base = std::filesystem::path(DL_ASSET_TEST_OUTPUT) / "startup_resolver";
			std::filesystem::path m_executable = m_base / "installed/bin/game";
			std::filesystem::path m_launch = m_base / "unrelated_launch";
			StartupPaths m_result{ "unchanged-root", "unchanged-config" };
			std::string m_error = "previous error";

			bool Resolve(const std::vector<std::filesystem::path>& arguments = {})
			{
				return StartupDiscovery::Resolve(m_executable, m_launch, arguments, m_result, m_error);
			}

			void ExpectUnchanged() const
			{
				EXPECT_EQ(m_result.runtime_root, "unchanged-root");
				EXPECT_EQ(m_result.path_config, "unchanged-config");
				EXPECT_FALSE(m_error.empty());
			}
		};
	}

	TEST_F(StartupDiscoveryTest, installed_bin_layout_selects_install_root)
	{
		ASSERT_TRUE(Resolve()) << m_error;
		EXPECT_EQ(m_result.runtime_root, m_base / "installed");
		EXPECT_EQ(m_result.path_config, m_base / "installed/path.ini");
		EXPECT_TRUE(m_error.empty());
	}

	TEST_F(StartupDiscoveryTest, multi_configuration_layouts_select_runtime_root)
	{
		for (const auto* platform : { "windows-x64", "linux-x64" })
		{
			for (const auto* configuration : { "Debug", "Release", "ReleaseDevTools" })
			{
				m_executable = m_base / "runtime/bin" / platform / configuration / "game";
				ASSERT_TRUE(Resolve()) << m_error;
				EXPECT_EQ(m_result.runtime_root, m_base / "runtime");
				EXPECT_EQ(m_result.path_config, m_base / "runtime/path.ini");
			}
		}
	}

	TEST_F(StartupDiscoveryTest, single_configuration_layouts_select_runtime_root)
	{
		for (const auto* platform : { "windows-x64", "linux-x64" })
		{
			m_executable = m_base / "runtime/bin" / platform / "game";
			ASSERT_TRUE(Resolve()) << m_error;
			EXPECT_EQ(m_result.runtime_root, m_base / "runtime");
		}
	}

	TEST_F(StartupDiscoveryTest, unrecognized_layout_defaults_beside_executable)
	{
		m_executable = m_base / "bin/other/deeper/game";
		ASSERT_TRUE(Resolve()) << m_error;
		EXPECT_EQ(m_result.runtime_root, m_executable.parent_path());
	}

	TEST_F(StartupDiscoveryTest, configuration_name_alone_does_not_trigger_layout_selection)
	{
		m_executable = m_base / "other/Release/game";
		ASSERT_TRUE(Resolve()) << m_error;
		EXPECT_EQ(m_result.runtime_root, m_executable.parent_path());
	}

	TEST_F(StartupDiscoveryTest, default_selection_is_independent_of_launch_directory)
	{
		ASSERT_TRUE(Resolve()) << m_error;
		const auto first = m_result;
		m_launch = m_base / "another/bin/a/b/c/d/e/f/g";
		ASSERT_TRUE(Resolve()) << m_error;
		EXPECT_EQ(m_result.runtime_root, first.runtime_root);
		EXPECT_EQ(m_result.path_config, first.path_config);
	}

	TEST_F(StartupDiscoveryTest, deeply_nested_install_layout_has_no_search_depth_limit)
	{
		m_executable = m_base / "a/b/c/d/e/f/g/install/bin/game";
		ASSERT_TRUE(Resolve()) << m_error;
		EXPECT_EQ(m_result.runtime_root, m_base / "a/b/c/d/e/f/g/install");
	}

	TEST_F(StartupDiscoveryTest, explicit_runtime_root_selects_its_default_config)
	{
		ASSERT_TRUE(Resolve({ "--runtime-root", m_base / "chosen" })) << m_error;
		EXPECT_EQ(m_result.runtime_root, m_base / "chosen");
		EXPECT_EQ(m_result.path_config, m_base / "chosen/path.ini");
	}

	TEST_F(StartupDiscoveryTest, config_only_override_selects_config_parent_as_root)
	{
		ASSERT_TRUE(Resolve({ "--path-config", "settings/custom.ini" })) << m_error;
		EXPECT_EQ(m_result.runtime_root, m_launch / "settings");
		EXPECT_EQ(m_result.path_config, m_launch / "settings/custom.ini");
	}

	TEST_F(StartupDiscoveryTest, both_overrides_are_launch_relative_and_order_independent)
	{
		for (const auto& arguments : {
			std::vector<std::filesystem::path>{ "--runtime-root", "runtime", "--path-config", "settings/custom.ini" },
			std::vector<std::filesystem::path>{ "--path-config", "settings/custom.ini", "--runtime-root", "runtime" } })
		{
			ASSERT_TRUE(Resolve(arguments)) << m_error;
			EXPECT_EQ(m_result.runtime_root, m_launch / "runtime");
			EXPECT_EQ(m_result.path_config, m_launch / "settings/custom.ini");
		}
	}

	TEST_F(StartupDiscoveryTest, explicit_overrides_normalize_dot_segments)
	{
		ASSERT_TRUE(Resolve({ "--runtime-root", "runtime/.././", "--path-config", "settings/../chosen.ini" })) << m_error;
		EXPECT_EQ(m_result.runtime_root, m_launch);
		EXPECT_EQ(m_result.path_config, m_launch / "chosen.ini");
	}

	TEST_F(StartupDiscoveryTest, native_unicode_and_spaced_paths_are_preserved)
	{
		const auto root = m_base / std::filesystem::path(u8"runtime 空間");
		const auto config = m_base / std::filesystem::path(u8"settings 空間/selected config.ini");
		ASSERT_TRUE(Resolve({ "--runtime-root", root, "--path-config", config })) << m_error;
		EXPECT_EQ(m_result.runtime_root, root);
		EXPECT_EQ(m_result.path_config, config);
	}

	TEST_F(StartupDiscoveryTest, missing_or_empty_values_leave_output_unchanged)
	{
		for (const auto& arguments : {
			std::vector<std::filesystem::path>{ "--runtime-root" },
			std::vector<std::filesystem::path>{ "--path-config", "" },
			std::vector<std::filesystem::path>{ "--runtime-root", "--path-config", "selected.ini" } })
		{
			EXPECT_FALSE(Resolve(arguments));
			ExpectUnchanged();
		}
	}

	TEST_F(StartupDiscoveryTest, duplicate_options_leave_output_unchanged)
	{
		for (const auto* option : { "--runtime-root", "--path-config" })
		{
			EXPECT_FALSE(Resolve({ option, "first", option, "second" }));
			ExpectUnchanged();
		}
	}

	TEST_F(StartupDiscoveryTest, unknown_options_leave_output_unchanged)
	{
		EXPECT_FALSE(Resolve({ "--runtime-root", "first", "--unknown", "value" }));
		ExpectUnchanged();
	}

	TEST_F(StartupDiscoveryTest, relative_executable_input_is_rejected)
	{
		m_executable = "relative/bin/game";
		EXPECT_FALSE(Resolve());
		ExpectUnchanged();
	}

	TEST_F(StartupDiscoveryTest, relative_launch_input_is_rejected)
	{
		m_launch = "relative-launch";
		EXPECT_FALSE(Resolve());
		ExpectUnchanged();
	}

#ifdef DL_PLATFORM_WINDOWS
	TEST_F(StartupDiscoveryTest, drive_relative_and_root_relative_overrides_are_rejected)
	{
		for (const auto* value : { "C:assets", "\\assets" })
		{
			EXPECT_FALSE(Resolve({ "--runtime-root", value }));
			ExpectUnchanged();
		}
	}
#endif
}
