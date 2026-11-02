#include <Utils/BundledFileInstaller.h>
#include <Utils/Utf8.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <iterator>

using Daedalus::BundledFileInstaller;

namespace
{
	struct BundledFiles
	{
		BundledFiles()
		{
			const auto* test = ::testing::UnitTest::GetInstance()->current_test_info();
			root = std::filesystem::path(DL_ASSET_TEST_OUTPUT) / test->test_suite_name() / test->name();
			std::filesystem::remove_all(root);
			std::filesystem::create_directories(root);
		}
		~BundledFiles()
		{
			std::error_code ignored;
			std::filesystem::remove_all(root, ignored);
		}
		bool Install(const std::string& path, const std::string& data)
		{
			return BundledFileInstaller::InstallMissing(root, path, [&data](std::ostream& output)
			{
				output.write(data.data(), static_cast<std::streamsize>(data.size()));
				return true;
			}, error);
		}
		std::string Read(const std::string& path) const
		{
			std::ifstream input(root / Daedalus::Utf8ToPath(path), std::ios::binary);
			return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
		}
		std::filesystem::path root;
		std::string error;
	};
}

TEST(BundledFileInstallerTest, installs_nested_binary_assets)
{
	BundledFiles files;
	const std::string data("model\0bytes", 11);
	ASSERT_TRUE(files.Install("asset/models/nested/model.bin", data)) << files.error;
	EXPECT_EQ(files.Read("asset/models/nested/model.bin"), data);
}

TEST(BundledFileInstallerTest, preserves_uuid_sidecars)
{
	BundledFiles files;
	ASSERT_TRUE(files.Install("asset/models/model.bin.meta", "DaedalusAsset 1\n123\n")) << files.error;
	EXPECT_EQ(files.Read("asset/models/model.bin.meta"), "DaedalusAsset 1\n123\n");
}

TEST(BundledFileInstallerTest, supports_utf8_filenames)
{
	BundledFiles files;
	const auto name = Daedalus::Utf8ToString(u8"asset/textures/камінь.png");
	ASSERT_TRUE(files.Install(name, "texture")) << files.error;
	EXPECT_EQ(files.Read(name), "texture");
}

TEST(BundledFileInstallerTest, preserves_existing_assets_without_reading_bundle)
{
	BundledFiles files;
	ASSERT_TRUE(files.Install("asset/scenes/startup.scene.json", "user scene")) << files.error;
	bool called = false;
	ASSERT_TRUE(BundledFileInstaller::InstallMissing(files.root, "asset/scenes/startup.scene.json",
		[&called](std::ostream&) { called = true; return false; }, files.error)) << files.error;
	EXPECT_FALSE(called);
	EXPECT_EQ(files.Read("asset/scenes/startup.scene.json"), "user scene");
}

TEST(BundledFileInstallerTest, preserves_existing_configuration)
{
	BundledFiles files;
	for (const auto* path : { "path.ini", "config/graphics.ini", "config/keybinds.ini" })
	{
		ASSERT_TRUE(files.Install(path, "user settings")) << files.error;
		ASSERT_TRUE(files.Install(path, "new APK settings")) << files.error;
		EXPECT_EQ(files.Read(path), "user settings");
	}
}

TEST(BundledFileInstallerTest, fills_missing_files_on_later_launches)
{
	BundledFiles files;
	ASSERT_TRUE(files.Install("asset/old.txt", "old")) << files.error;
	ASSERT_TRUE(files.Install("asset/old.txt", "replacement")) << files.error;
	ASSERT_TRUE(files.Install("asset/new.txt", "new")) << files.error;
	EXPECT_EQ(files.Read("asset/old.txt"), "old");
	EXPECT_EQ(files.Read("asset/new.txt"), "new");
}

TEST(BundledFileInstallerTest, failed_reads_do_not_publish_partial_files)
{
	BundledFiles files;
	EXPECT_FALSE(BundledFileInstaller::InstallMissing(files.root, "asset/model.bin",
		[](std::ostream& output) { output << "partial"; return false; }, files.error));
	EXPECT_FALSE(files.error.empty());
	EXPECT_FALSE(std::filesystem::exists(files.root / "asset/model.bin"));
	for (const auto& entry : std::filesystem::directory_iterator(files.root))
		EXPECT_EQ(entry.path().filename(), "asset");
	ASSERT_TRUE(files.Install("asset/model.bin", "complete")) << files.error;
	EXPECT_EQ(files.Read("asset/model.bin"), "complete");
}

TEST(BundledFileInstallerTest, failed_writes_do_not_publish_partial_files)
{
	BundledFiles files;
	EXPECT_FALSE(BundledFileInstaller::InstallMissing(files.root, "asset/model.bin",
		[](std::ostream& output) { output.setstate(std::ios::badbit); return true; }, files.error));
	EXPECT_FALSE(std::filesystem::exists(files.root / "asset/model.bin"));
}

TEST(BundledFileInstallerTest, rejects_unsafe_or_unsupported_paths)
{
	BundledFiles files;
	for (const std::string path : { "", "/asset/file", "../outside", "asset/../outside", "asset/./file",
		"asset//file", "asset/", "C:/asset/file", "asset\\file", "shader/file.glsl", "logs/file", "asset", "config", "asset/file\nname" })
	{
		bool called = false;
		EXPECT_FALSE(BundledFileInstaller::InstallMissing(files.root, path,
			[&called](std::ostream&) { called = true; return true; }, files.error)) << path;
		EXPECT_FALSE(files.error.empty()) << path;
		EXPECT_FALSE(called) << path;
	}
	EXPECT_TRUE(std::filesystem::is_empty(files.root));
}

TEST(BundledFileInstallerTest, rejects_directory_at_file_destination)
{
	BundledFiles files;
	std::filesystem::create_directories(files.root / "asset/model.bin");
	EXPECT_FALSE(files.Install("asset/model.bin", "data"));
	EXPECT_TRUE(std::filesystem::is_directory(files.root / "asset/model.bin"));
}

TEST(BundledFileInstallerTest, rejects_file_at_parent_destination)
{
	BundledFiles files;
	ASSERT_TRUE(files.Install("asset/model", "existing")) << files.error;
	EXPECT_FALSE(files.Install("asset/model/data.bin", "data"));
	EXPECT_EQ(files.Read("asset/model"), "existing");
}

TEST(BundledFileInstallerTest, rejects_symlink_destinations)
{
	BundledFiles files;
	std::filesystem::create_directories(files.root / "outside");
	std::error_code error;
	std::filesystem::create_directory_symlink(files.root / "outside", files.root / "asset", error);
	if (error) GTEST_SKIP() << "Symlink creation unavailable: " << error.message();
	EXPECT_FALSE(files.Install("asset/escaped.txt", "data"));
	EXPECT_FALSE(std::filesystem::exists(files.root / "outside/escaped.txt"));
}

TEST(BundledFileInstallerTest, does_not_replace_files_created_during_extraction)
{
	BundledFiles files;
	ASSERT_TRUE(BundledFileInstaller::InstallMissing(files.root, "asset/model.bin",
		[&files](std::ostream& output)
		{
			std::ofstream existing(files.root / "asset/model.bin");
			existing << "user data";
			output << "APK data";
			return true;
		}, files.error)) << files.error;
	EXPECT_EQ(files.Read("asset/model.bin"), "user data");
}

TEST(BundledFileInstallerTest, installs_empty_files)
{
	BundledFiles files;
	ASSERT_TRUE(files.Install("asset/empty", "")) << files.error;
	EXPECT_EQ(std::filesystem::file_size(files.root / "asset/empty"), 0u);
}
