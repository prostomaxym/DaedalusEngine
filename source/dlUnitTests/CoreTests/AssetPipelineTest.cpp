#include <Core/AssetPipeline.h>
#include <Utils/Utf8.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <thread>

using Daedalus::AssetId;
using Daedalus::AssetLoadMode;
using Daedalus::AssetLoadResult;
using Daedalus::AssetPipeline;
using Daedalus::AssetRegistry;
using Daedalus::JobSystem;
using Daedalus::VirtualFileSystem;

namespace
{
	class AssetTestFiles
	{
	public:
		AssetTestFiles()
		{
			const auto* test = ::testing::UnitTest::GetInstance()->current_test_info();
			m_root = std::filesystem::path(DL_ASSET_TEST_OUTPUT) / test->test_suite_name() / test->name();
			std::filesystem::remove_all(m_root);
			std::filesystem::create_directories(m_root / "assets");
			std::filesystem::create_directories(m_root / "shaders");
		}
		~AssetTestFiles()
		{
			std::error_code ec;
			std::filesystem::remove_all(m_root, ec);
		}
		void Write(const std::string& path, const std::string& data)
		{
			const auto destination = m_root / std::filesystem::u8path(path);
			std::filesystem::create_directories(destination.parent_path());
			std::ofstream output(destination, std::ios::binary);
			output.write(data.data(), static_cast<std::streamsize>(data.size()));
		}
		VirtualFileSystem Mounts() const
		{
			VirtualFileSystem files;
			EXPECT_TRUE(files.Mount("assets", m_root / "assets"));
			EXPECT_TRUE(files.Mount("shaders", m_root / "shaders"));
			return files;
		}
		std::filesystem::path m_root;
	};

	AssetId FindId(const AssetPipeline& pipeline, const std::string& path)
	{
		const auto* metadata = pipeline.GetRegistry().Find(path);
		EXPECT_NE(metadata, nullptr);
		return metadata ? metadata->id : 0;
	}
}

TEST(VirtualFileSystemTest, resolves_named_mounts_and_rejects_unknown_paths)
{
	AssetTestFiles fixture;
	const auto files = fixture.Mounts();
	const auto path = files.Resolve("assets:/models/example.obj");
	ASSERT_TRUE(path);
	EXPECT_EQ(*path, std::filesystem::weakly_canonical(fixture.m_root / "assets/models/example.obj"));
	EXPECT_TRUE(files.Resolve("assets:/"));
	EXPECT_FALSE(files.Resolve("unknown:/file"));
	EXPECT_FALSE(files.Resolve("assets/file"));
}

TEST(VirtualFileSystemTest, rejects_traversal_absolute_paths_and_invalid_mount_names)
{
	AssetTestFiles fixture;
	auto files = fixture.Mounts();
	for (const auto* path : { "assets:/../outside", "assets:/inside/../../outside", "assets://outside", "assets:/C:/outside", "assets:/..\\outside" })
		EXPECT_FALSE(files.Resolve(path)) << path;
	EXPECT_FALSE(files.Mount("bad/name", fixture.m_root));
	EXPECT_FALSE(files.Mount("", fixture.m_root));
	EXPECT_FALSE(files.Mount("empty", {}));
}

TEST(VirtualFileSystemTest, enumerates_sources_deterministically_without_metadata_or_cache)
{
	AssetTestFiles fixture;
	fixture.Write("assets/z.txt", "z");
	fixture.Write("assets/a.txt", "a");
	fixture.Write("assets/a.txt.meta", "metadata");
	fixture.Write("assets/cache/ignored.bin", "cache");
	std::string error;
	EXPECT_EQ(fixture.Mounts().List("assets", error), (std::vector<std::string>{ "assets:/a.txt", "assets:/z.txt" }));
	EXPECT_TRUE(error.empty());
}

TEST(VirtualFileSystemTest, missing_mount_directory_is_an_empty_listing)
{
	AssetTestFiles fixture;
	VirtualFileSystem files;
	ASSERT_TRUE(files.Mount("assets", fixture.m_root / "missing"));
	std::string error;
	EXPECT_TRUE(files.List("assets", error).empty());
	EXPECT_TRUE(error.empty());
}

TEST(VirtualFileSystemTest, preserves_utf8_virtual_paths)
{
	AssetTestFiles fixture;
	const std::string name = "caf\xc3\xa9.txt";
	fixture.Write("assets/" + name, "text");
	const auto files = fixture.Mounts();
	const auto resolved = files.Resolve("assets:/" + name);
	ASSERT_TRUE(resolved);
	EXPECT_EQ(Daedalus::Utf8ToString(resolved->filename().generic_u8string()), name);
	std::string error;
	EXPECT_EQ(files.List("assets", error), (std::vector<std::string>{ "assets:/" + name }));
}

TEST(VirtualFileSystemTest, rejects_symlinks_escaping_the_mount)
{
	AssetTestFiles fixture;
	std::filesystem::create_directories(fixture.m_root / "outside");
	std::error_code ec;
	std::filesystem::create_directory_symlink(fixture.m_root / "outside", fixture.m_root / "assets/link", ec);
	if (ec) GTEST_SKIP() << "Symlink creation unavailable: " << ec.message();
	EXPECT_FALSE(fixture.Mounts().Resolve("assets:/link/file.txt"));
}

TEST(AssetRegistryTest, creates_persistent_nonzero_uuid_sidecars)
{
	AssetTestFiles fixture;
	fixture.Write("assets/example.txt", "example");
	AssetRegistry registry;
	ASSERT_TRUE(registry.Scan(fixture.Mounts()));
	const auto* metadata = registry.Find("assets:/example.txt");
	ASSERT_NE(metadata, nullptr);
	const auto id = metadata->id;
	EXPECT_NE(id, 0u);
	EXPECT_TRUE(std::filesystem::is_regular_file(fixture.m_root / "assets/example.txt.meta"));
	ASSERT_TRUE(registry.Scan(fixture.Mounts()));
	ASSERT_NE(registry.Find(id), nullptr);
	EXPECT_EQ(registry.Find(id)->virtual_path, "assets:/example.txt");
}

TEST(AssetRegistryTest, preserves_identity_when_source_and_sidecar_are_renamed)
{
	AssetTestFiles fixture;
	fixture.Write("assets/old.txt", "old");
	AssetRegistry registry;
	ASSERT_TRUE(registry.Scan(fixture.Mounts()));
	const auto id = registry.Find("assets:/old.txt")->id;
	std::filesystem::rename(fixture.m_root / "assets/old.txt", fixture.m_root / "assets/new.txt");
	std::filesystem::rename(fixture.m_root / "assets/old.txt.meta", fixture.m_root / "assets/new.txt.meta");
	ASSERT_TRUE(registry.Scan(fixture.Mounts()));
	EXPECT_EQ(registry.Find("assets:/old.txt"), nullptr);
	ASSERT_NE(registry.Find(id), nullptr);
	EXPECT_EQ(registry.Find(id)->virtual_path, "assets:/new.txt");
}

TEST(AssetRegistryTest, preserves_identity_when_mount_directory_is_relocated)
{
	AssetTestFiles fixture;
	fixture.Write("assets/file.txt", "text");
	AssetRegistry registry;
	ASSERT_TRUE(registry.Scan(fixture.Mounts()));
	const auto id = registry.Find("assets:/file.txt")->id;
	std::filesystem::rename(fixture.m_root / "assets", fixture.m_root / "relocated");
	VirtualFileSystem files;
	ASSERT_TRUE(files.Mount("assets", fixture.m_root / "relocated"));
	ASSERT_TRUE(registry.Scan(files));
	ASSERT_NE(registry.Find(id), nullptr);
	EXPECT_EQ(registry.Find(id)->source_path, std::filesystem::weakly_canonical(fixture.m_root / "relocated/file.txt"));
}

TEST(AssetRegistryTest, rejects_all_assets_with_duplicate_identities)
{
	AssetTestFiles fixture;
	for (const auto* name : { "a.txt", "b.txt", "c.txt" })
	{
		fixture.Write(std::string("assets/") + name, "text");
		fixture.Write(std::string("assets/") + name + ".meta", "DaedalusAsset 1\n42\n");
	}
	AssetRegistry registry;
	EXPECT_FALSE(registry.Scan(fixture.Mounts()));
	EXPECT_TRUE(registry.GetAssets().empty());
	EXPECT_EQ(registry.Find(42), nullptr);
	EXPECT_EQ(registry.GetErrors().size(), 3u);
}

TEST(AssetRegistryTest, rejects_invalid_metadata_without_overwriting_it)
{
	AssetTestFiles fixture;
	fixture.Write("assets/file.txt", "text");
	for (const auto* metadata : { "invalid\n42\n", "DaedalusAsset 1\n0\n", "DaedalusAsset 1\n18446744073709551616\n", "DaedalusAsset 1\n42junk\n", "DaedalusAsset 2\n42\n", "DaedalusAsset 1\n42\nextra\n" })
	{
		fixture.Write("assets/file.txt.meta", metadata);
		AssetRegistry registry;
		EXPECT_FALSE(registry.Scan(fixture.Mounts()));
		EXPECT_TRUE(registry.GetAssets().empty());
		std::ifstream input(fixture.m_root / "assets/file.txt.meta", std::ios::binary);
		const std::string actual((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
		EXPECT_EQ(actual, metadata);
	}
}

TEST(AssetRegistryTest, reports_metadata_creation_failure)
{
	AssetTestFiles fixture;
	fixture.Write("assets/file.txt", "text");
	std::filesystem::create_directory(fixture.m_root / "assets/file.txt.meta");
	AssetRegistry registry;
	EXPECT_FALSE(registry.Scan(fixture.Mounts()));
	EXPECT_EQ(registry.Find("assets:/file.txt"), nullptr);
	EXPECT_FALSE(registry.GetErrors().empty());
}

TEST(AssetRegistryTest, accepts_crlf_metadata_and_maximum_uuid)
{
	AssetTestFiles fixture;
	fixture.Write("assets/file.txt", "text");
	fixture.Write("assets/file.txt.meta", "DaedalusAsset 1\r\n18446744073709551615\r\n");
	AssetRegistry registry;
	ASSERT_TRUE(registry.Scan(fixture.Mounts()));
	EXPECT_NE(registry.Find(UINT64_MAX), nullptr);
}

TEST(AssetRegistryTest, indexes_only_asset_and_shader_mounts)
{
	AssetTestFiles fixture;
	fixture.Write("assets/file.txt", "asset");
	fixture.Write("shaders/test.glsl", "shader");
	fixture.Write("config/config.ini", "config");
	auto files = fixture.Mounts();
	ASSERT_TRUE(files.Mount("config", fixture.m_root / "config"));
	AssetRegistry registry;
	ASSERT_TRUE(registry.Scan(files));
	EXPECT_EQ(registry.GetAssets().size(), 2u);
	EXPECT_FALSE(std::filesystem::exists(fixture.m_root / "config/config.ini.meta"));
}

TEST(AssetRegistryTest, rejects_sidecar_symlinks_escaping_the_mount)
{
	AssetTestFiles fixture;
	fixture.Write("assets/file.txt", "text");
	fixture.Write("outside.meta", "DaedalusAsset 1\n42\n");
	std::error_code ec;
	std::filesystem::create_symlink(fixture.m_root / "outside.meta", fixture.m_root / "assets/file.txt.meta", ec);
	if (ec) GTEST_SKIP() << "Symlink creation unavailable: " << ec.message();
	AssetRegistry registry;
	EXPECT_FALSE(registry.Scan(fixture.Mounts()));
	EXPECT_TRUE(registry.GetAssets().empty());
}

TEST(AssetPipelineTest, reads_binary_bytes_before_owner_thread_callback)
{
	AssetTestFiles fixture;
	const std::string bytes("a\0b", 3);
	fixture.Write("assets/file.bin", bytes);
	JobSystem jobs(2);
	AssetPipeline pipeline(jobs, fixture.Mounts());
	ASSERT_TRUE(pipeline.Initialize());
	bool called = false;
	const auto owner = std::this_thread::get_id();
	const auto handle = pipeline.LoadAsync(FindId(pipeline, "assets:/file.bin"), [&](AssetLoadResult result)
	{
		called = true;
		EXPECT_EQ(std::this_thread::get_id(), owner);
		EXPECT_TRUE(result.Succeeded());
		EXPECT_EQ(result.data, bytes);
		EXPECT_EQ(result.virtual_path, "assets:/file.bin");
	});
	EXPECT_FALSE(called);
	EXPECT_TRUE(jobs.Wait(handle));
	EXPECT_TRUE(called);
}

TEST(AssetPipelineTest, reports_unknown_asset_identity_through_callback)
{
	AssetTestFiles fixture;
	JobSystem jobs(1);
	AssetPipeline pipeline(jobs, fixture.Mounts());
	ASSERT_TRUE(pipeline.Initialize());
	bool called = false;
	const auto handle = pipeline.LoadAsync(0, [&](AssetLoadResult result)
	{
		called = true;
		EXPECT_FALSE(result.Succeeded());
		EXPECT_TRUE(result.data.empty());
	});
	EXPECT_TRUE(jobs.Wait(handle));
	EXPECT_TRUE(called);
}

TEST(AssetPipelineTest, reports_source_deleted_after_registration)
{
	AssetTestFiles fixture;
	fixture.Write("assets/file.txt", "text");
	JobSystem jobs(1);
	AssetPipeline pipeline(jobs, fixture.Mounts());
	ASSERT_TRUE(pipeline.Initialize());
	const auto id = FindId(pipeline, "assets:/file.txt");
	std::filesystem::remove(fixture.m_root / "assets/file.txt");
	bool called = false;
	const auto handle = pipeline.LoadAsync(id, [&](AssetLoadResult result)
	{
		called = true;
		EXPECT_FALSE(result.Succeeded());
	});
	EXPECT_TRUE(jobs.Wait(handle));
	EXPECT_TRUE(called);
}

TEST(AssetPipelineTest, discovers_additions_modifications_and_deletions_on_refresh)
{
	AssetTestFiles fixture;
	fixture.Write("assets/old.txt", "old");
	JobSystem jobs(2);
	AssetPipeline pipeline(jobs, fixture.Mounts());
	ASSERT_TRUE(pipeline.Initialize());
	std::vector<std::string> changed;
	pipeline.SetChangeCallback([&](const auto& paths) { changed = paths; });
	const auto old_time = std::filesystem::last_write_time(fixture.m_root / "assets/old.txt");
	fixture.Write("assets/old.txt", "changed");
	std::filesystem::last_write_time(fixture.m_root / "assets/old.txt", old_time + std::chrono::seconds(2));
	fixture.Write("assets/new.txt", "new");
	EXPECT_TRUE(jobs.Wait(pipeline.Refresh()));
	EXPECT_EQ(changed, (std::vector<std::string>{ "assets:/new.txt", "assets:/old.txt" }));
	changed.clear();
	EXPECT_TRUE(jobs.Wait(pipeline.Refresh()));
	EXPECT_TRUE(changed.empty());
	std::filesystem::remove(fixture.m_root / "assets/old.txt");
	EXPECT_TRUE(jobs.Wait(pipeline.Refresh()));
	EXPECT_EQ(changed, (std::vector<std::string>{ "assets:/old.txt" }));
	EXPECT_EQ(pipeline.GetRegistry().Find("assets:/old.txt"), nullptr);
}

TEST(AssetPipelineTest, detects_header_changes_without_graphics_context)
{
	AssetTestFiles fixture;
	fixture.Write("shaders/shared.hglsl", "old header");
	JobSystem jobs(1);
	AssetPipeline pipeline(jobs, fixture.Mounts());
	ASSERT_TRUE(pipeline.Initialize());
	std::vector<std::string> changed;
	pipeline.SetChangeCallback([&](const auto& paths) { changed = paths; });
	fixture.Write("shaders/shared.hglsl", "different header");
	EXPECT_TRUE(jobs.Wait(pipeline.Refresh()));
	EXPECT_EQ(changed, (std::vector<std::string>{ "shaders:/shared.hglsl" }));
}

TEST(AssetPipelineTest, expands_nested_shader_includes_and_crlf_on_workers)
{
	AssetTestFiles fixture;
	fixture.Write("shaders/test.glsl", "#vertex\r\n#include \"shared.hglsl\"\r\nvoid main() {}\r\n");
	fixture.Write("shaders/shared.hglsl", "#include \"nested.hglsl\"\r\nfloat shared;\r\n");
	fixture.Write("shaders/nested.hglsl", "float nested;\r\n");
	JobSystem jobs(2);
	AssetPipeline pipeline(jobs, fixture.Mounts());
	ASSERT_TRUE(pipeline.Initialize());
	bool called = false;
	const auto handle = pipeline.LoadAsync(FindId(pipeline, "shaders:/test.glsl"), [&](AssetLoadResult result)
	{
		called = true;
		EXPECT_TRUE(result.Succeeded()) << result.error;
		EXPECT_EQ(result.data, "#vertex\nfloat nested;\nfloat shared;\nvoid main() {}\n");
	}, AssetLoadMode::ShaderSource);
	EXPECT_TRUE(jobs.Wait(handle));
	EXPECT_TRUE(called);
}

TEST(AssetPipelineTest, rejects_shader_include_cycles_missing_files_and_traversal)
{
	AssetTestFiles fixture;
	fixture.Write("shaders/test.glsl", "#vertex\n");
	fixture.Write("shaders/cycle.hglsl", "#include \"test.glsl\"\n");
	JobSystem jobs(1);
	AssetPipeline pipeline(jobs, fixture.Mounts());
	ASSERT_TRUE(pipeline.Initialize());
	const auto id = FindId(pipeline, "shaders:/test.glsl");
	for (const auto* include : { "cycle.hglsl", "missing.hglsl", "../outside.hglsl", "", "C:/outside.hglsl" })
	{
		fixture.Write("shaders/test.glsl", std::string("#vertex\n#include \"") + include + "\"\n");
		bool called = false;
		const auto handle = pipeline.LoadAsync(id, [&](AssetLoadResult result)
		{
			called = true;
			EXPECT_FALSE(result.Succeeded()) << include;
			EXPECT_TRUE(result.data.empty());
		}, AssetLoadMode::ShaderSource);
		EXPECT_TRUE(jobs.Wait(handle));
		EXPECT_TRUE(called);
	}
}

TEST(AssetPipelineTest, supports_include_paths_with_spaces_and_trailing_comments)
{
	AssetTestFiles fixture;
	fixture.Write("shaders/test.glsl", "#vertex\n  #include \"shared header.hglsl\" // shared declarations\n");
	fixture.Write("shaders/shared header.hglsl", "float shared;\n");
	JobSystem jobs(1);
	AssetPipeline pipeline(jobs, fixture.Mounts());
	ASSERT_TRUE(pipeline.Initialize());
	const auto handle = pipeline.LoadAsync(FindId(pipeline, "shaders:/test.glsl"), [&](AssetLoadResult result)
	{
		EXPECT_TRUE(result.Succeeded()) << result.error;
		EXPECT_EQ(result.data, "#vertex\nfloat shared;\n");
	}, AssetLoadMode::ShaderSource);
	EXPECT_TRUE(jobs.Wait(handle));
}

TEST(AssetPipelineTest, drains_accepted_loads_during_scheduler_shutdown)
{
	AssetTestFiles fixture;
	fixture.Write("assets/file.txt", "text");
	JobSystem jobs(2);
	AssetPipeline pipeline(jobs, fixture.Mounts());
	ASSERT_TRUE(pipeline.Initialize());
	bool called = false;
	const auto handle = pipeline.LoadAsync(FindId(pipeline, "assets:/file.txt"), [&](AssetLoadResult result)
	{
		called = true;
		EXPECT_TRUE(result.Succeeded());
	});
	jobs.Stop();
	EXPECT_TRUE(handle.IsComplete());
	EXPECT_FALSE(handle.HasFailed());
	EXPECT_TRUE(called);
}

TEST(AssetPipelineTest, rejects_loads_after_shutdown_without_invoking_callback)
{
	AssetTestFiles fixture;
	JobSystem jobs(1);
	AssetPipeline pipeline(jobs, fixture.Mounts());
	ASSERT_TRUE(pipeline.Initialize());
	jobs.Stop();
	bool called = false;
	const auto handle = pipeline.LoadAsync(0, [&](AssetLoadResult) { called = true; });
	EXPECT_TRUE(handle.IsComplete());
	EXPECT_TRUE(handle.HasFailed());
	EXPECT_FALSE(called);
}

TEST(AssetPipelineTest, drains_callbacks_before_pipeline_destruction)
{
	AssetTestFiles fixture;
	fixture.Write("assets/file.txt", "text");
	JobSystem jobs(1);
	bool called = false;
	{
		AssetPipeline pipeline(jobs, fixture.Mounts());
		ASSERT_TRUE(pipeline.Initialize());
		(void)pipeline.LoadAsync(FindId(pipeline, "assets:/file.txt"), [&](AssetLoadResult result)
		{
			called = true;
			EXPECT_TRUE(result.Succeeded());
		});
	}
	EXPECT_TRUE(called);
}
