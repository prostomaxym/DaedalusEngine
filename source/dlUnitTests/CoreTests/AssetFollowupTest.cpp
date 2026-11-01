#include <Core/AssetPipeline.h>
#include <Renderer/Resources/AssetImporter.h>
#include <Renderer/Resources/ResourceManager.h>
#include <ECS/Scene.h>
#include <ECS/Serialization/SceneJson.h>
#include <ECS/Serialization/SceneSerializer.h>
#include <ECS/Serialization/ScriptRegistry.h>
#include <Utils/AssetSourceReader.h>
#include <Utils/ContentFingerprint.h>
#include <Utils/Utf8.h>
#include <gtest/gtest.h>
#include <zip.h>
#include <fstream>
#include <array>
#include <thread>

using namespace Daedalus;

namespace
{
	struct FollowupFiles
	{
		FollowupFiles()
		{
			const auto* test = ::testing::UnitTest::GetInstance()->current_test_info();
			root = std::filesystem::path(DL_ASSET_TEST_OUTPUT) / test->test_suite_name() / test->name();
			std::filesystem::remove_all(root);
			std::filesystem::create_directories(root / "assets");
		}
		~FollowupFiles() { std::error_code ec; std::filesystem::remove_all(root, ec); }
		void Write(const std::string& name, const std::string& bytes)
		{
			const auto path = root / Utf8ToPath(name);
			std::filesystem::create_directories(path.parent_path());
			std::ofstream output(path, std::ios::binary);
			output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
		}
		bool Zip(const std::map<std::string, std::string>& entries, bool metadata = true, bool symlink = false, bool compressed = false)
		{
			auto contents = entries;
			AssetId id = 100;
			if (metadata) for (const auto& [name, data] : entries)
			{
				// Directory records are not asset sources and must not get UUID sidecars.
				if (!name.empty() && (name.back() == '/' || name.back() == '\\')) continue;
				contents[name + ".meta"] = "DaedalusAsset 1\n" + std::to_string(id++) + "\n";
			}
			int error = 0;
			std::unique_ptr<zip_t, decltype(&zip_discard)> archive(zip_open((root / "assets.zip").string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error), &zip_discard);
			if (!archive) return false;
			for (const auto& [name, data] : contents)
			{
				auto* source = zip_source_buffer(archive.get(), data.data(), data.size(), 0);
				if (!source) return false;
				const auto index = zip_file_add(archive.get(), name.c_str(), source, ZIP_FL_ENC_UTF_8);
				if (index < 0) { zip_source_free(source); return false; }
				if (zip_set_file_compression(archive.get(), static_cast<zip_uint64_t>(index), compressed ? ZIP_CM_DEFLATE : ZIP_CM_STORE, 0) != 0 ||
					zip_file_set_mtime(archive.get(), static_cast<zip_uint64_t>(index), 123456, 0) != 0) return false;
				if (symlink && zip_file_set_external_attributes(archive.get(), static_cast<zip_uint64_t>(index), 0, ZIP_OPSYS_UNIX, 0120777u << 16) != 0) return false;
			}
			auto* raw = archive.release();
			if (zip_close(raw) != 0) { zip_discard(raw); return false; }
			return true;
		}
		VirtualFileSystem Directory() const
		{
			VirtualFileSystem files;
			EXPECT_TRUE(files.Mount("assets", root / "assets"));
			return files;
		}
		VirtualFileSystem Archive(const std::string& mount = "assets") const
		{
			VirtualFileSystem files;
			std::string error;
			EXPECT_TRUE(files.MountArchive(mount, root / "assets.zip", error)) << error;
			return files;
		}
		std::filesystem::path root;
	};

	std::string Pixel(unsigned char red, unsigned char green, unsigned char blue)
	{
		std::string bytes(18, '\0');
		bytes[2] = 2; bytes[12] = 1; bytes[14] = 1; bytes[16] = 24;
		bytes.push_back(static_cast<char>(blue)); bytes.push_back(static_cast<char>(green)); bytes.push_back(static_cast<char>(red));
		return bytes;
	}
	const std::string triangle = "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
}

TEST(ArchiveMountTest, reads_compressed_binary_and_utf8_entries_with_limits)
{
	FollowupFiles fixture;
	const std::string name = "caf\xc3\xa9.bin";
	const std::string bytes("a\0b", 3);
	ASSERT_TRUE(fixture.Zip({ { name, bytes } }, true, false, true));
	const auto files = fixture.Archive();
	std::string data, error;
	EXPECT_TRUE(files.Read("assets:/" + name, data, error)) << error;
	EXPECT_EQ(data, bytes);
	EXPECT_FALSE(files.Read("assets:/" + name, data, error, 2));
	EXPECT_TRUE(data.empty());
	EXPECT_FALSE(files.Resolve("assets:/" + name));
}

TEST(ArchiveMountTest, rejects_unsafe_paths_without_replacing_a_working_mount)
{
	FollowupFiles fixture;
	auto files = fixture.Directory();
	for (const auto* name : { "../outside.txt", "/absolute.txt", "C:/file.txt", "folder/./file.txt",
		"..\\outside.txt", "\\absolute.txt", "C:\\file.txt", "folder\\.\\file.txt", "folder\\..\\outside.txt", "folder\\\\file.txt" })
	{
		ASSERT_TRUE(fixture.Zip({ { name, "unsafe" } }, false));
		std::string error;
		EXPECT_FALSE(files.MountArchive("assets", fixture.root / "assets.zip", error)) << name;
		EXPECT_FALSE(error.empty());
		EXPECT_FALSE(files.IsArchive("assets"));
	}
}

TEST(ArchiveMountTest, rejects_archived_symlinks_without_os_symlink_privileges)
{
	FollowupFiles fixture;
	ASSERT_TRUE(fixture.Zip({ { "link", "outside" } }, false, true));
	VirtualFileSystem files;
	std::string error;
	EXPECT_FALSE(files.MountArchive("assets", fixture.root / "assets.zip", error));
}

TEST(ArchiveMountTest, enumerates_sources_without_extracting_cache_or_metadata)
{
	FollowupFiles fixture;
	ASSERT_TRUE(fixture.Zip({ { "a.txt", "a" }, { "cache/ignored.bin", "cache" }, { "z.txt", "z" } }));
	std::string error;
	EXPECT_EQ(fixture.Archive().List("assets", error), (std::vector<std::string>{ "assets:/a.txt", "assets:/z.txt" }));
	EXPECT_FALSE(std::filesystem::exists(fixture.root / "assets/a.txt"));
}

TEST(ArchiveMountTest, mounting_a_configured_zip_path_selects_archive_reads)
{
	FollowupFiles fixture;
	ASSERT_TRUE(fixture.Zip({ { "a.txt", "a" } }));
	VirtualFileSystem files;
	EXPECT_TRUE(files.Mount("assets", fixture.root / "assets.zip"));
	EXPECT_TRUE(files.IsArchive("assets"));
}

TEST(ArchiveMountTest, reads_and_lists_windows_zip_names_as_canonical_virtual_paths)
{
	FollowupFiles fixture;
	const std::string bytes("font\0bytes", 10);
	ASSERT_TRUE(fixture.Zip({ { "fonts\\calibrib.ttf", bytes }, { "models/mixed\\a.obj", triangle },
		{ "empty\\", "" }, { "cache\\ignored.bin", "cache" } }, true, false, true));
	const auto files = fixture.Archive();
	std::string data, error;
	EXPECT_TRUE(files.Read("assets:/fonts/calibrib.ttf", data, error)) << error;
	EXPECT_EQ(data, bytes);
	EXPECT_TRUE(files.Read("assets:/models/mixed/a.obj", data, error)) << error;
	EXPECT_EQ(data, triangle);
	EXPECT_TRUE(files.Read("assets:/fonts/calibrib.ttf.meta", data, error)) << error;
	EXPECT_FALSE(files.Read("assets:/fonts\\calibrib.ttf", data, error));
	EXPECT_EQ(files.List("assets", error), (std::vector<std::string>{ "assets:/fonts/calibrib.ttf", "assets:/models/mixed/a.obj" }));
	EXPECT_TRUE(error.empty());
}

TEST(ArchiveMountTest, rejects_separator_aliases_without_replacing_a_working_mount)
{
	FollowupFiles fixture;
	auto files = fixture.Directory();
	for (const auto& entries : { std::map<std::string, std::string>{ { "fonts/a.ttf", "a" }, { "fonts\\a.ttf", "b" } },
		std::map<std::string, std::string>{ { "fonts/", "" }, { "fonts\\", "" } } })
	{
		ASSERT_TRUE(fixture.Zip(entries, false));
		std::string error;
		EXPECT_FALSE(files.Mount("assets", fixture.root / "assets.zip", error));
		EXPECT_NE(error.find("Duplicate canonical ZIP entry"), std::string::npos);
		EXPECT_FALSE(files.IsArchive("assets"));
	}
}

TEST(ArchiveMountTest, resolves_current_indices_and_revalidates_repacked_archives)
{
	FollowupFiles fixture;
	ASSERT_TRUE(fixture.Zip({ { "folder\\a.txt", "original" } }, false));
	const auto files = fixture.Archive();
	ASSERT_TRUE(fixture.Zip({ { "before.txt", "before" }, { "folder/a.txt", "updated" } }, false));
	std::string data, error;
	EXPECT_TRUE(files.Read("assets:/folder/a.txt", data, error)) << error;
	EXPECT_EQ(data, "updated");
	ASSERT_TRUE(fixture.Zip({ { "folder/a.txt", "a" }, { "folder\\a.txt", "alias" } }, false));
	EXPECT_FALSE(files.Read("assets:/folder/a.txt", data, error));
	EXPECT_TRUE(data.empty());
	EXPECT_NE(error.find("Duplicate canonical ZIP entry"), std::string::npos);
}

TEST(ArchiveMountTest, configured_mount_errors_describe_missing_and_unsafe_archives)
{
	FollowupFiles fixture;
	auto files = fixture.Directory();
	std::string error;
	EXPECT_FALSE(files.Mount("assets", fixture.root / "missing.zip", error));
	EXPECT_NE(error.find("Cannot open ZIP archive"), std::string::npos);
	ASSERT_TRUE(fixture.Zip({ { "..\\outside.txt", "unsafe" } }, false));
	EXPECT_FALSE(files.Mount("assets", fixture.root / "assets.zip", error));
	EXPECT_NE(error.find("Unsafe ZIP entry path"), std::string::npos);
	EXPECT_FALSE(files.IsArchive("assets"));
}

TEST(ArchiveRegistryTest, reads_bundled_identities_from_windows_zip_names)
{
	FollowupFiles fixture;
	ASSERT_TRUE(fixture.Zip({ { "fonts\\calibrib.ttf", "font" } }));
	AssetRegistry registry;
	ASSERT_TRUE(registry.Scan(fixture.Archive()));
	const auto* entry = registry.Find("assets:/fonts/calibrib.ttf");
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->id, 100u);
	EXPECT_EQ(entry->content_hash, ContentFingerprint("font"));
}

TEST(ArchiveMountTest, scan_reader_is_bounded_canonical_and_excludes_cache_and_metadata)
{
	FollowupFiles fixture;
	ASSERT_TRUE(fixture.Zip({ { "fonts\\a.ttf", "font" }, { "cache\\ignored.bin", "cache" } }));
	const auto files = fixture.Archive();
	std::vector<std::string> paths;
	std::string error;
	EXPECT_TRUE(files.VisitSources("assets", [&](const std::string& path, const VirtualFileSystem::ReadCallback& reader)
	{
		paths.push_back(path);
		std::string data, read_error;
		EXPECT_TRUE(reader(path, data, read_error, 4)) << read_error;
		EXPECT_EQ(data, "font");
		EXPECT_TRUE(reader(path + ".meta", data, read_error, 4096)) << read_error;
		EXPECT_FALSE(reader(path, data, read_error, 3));
		EXPECT_TRUE(data.empty());
		EXPECT_FALSE(reader("shaders:/fonts/a.ttf", data, read_error, 4));
		EXPECT_FALSE(reader("assets:/../outside", data, read_error, 4));
	}, error)) << error;
	EXPECT_EQ(paths, (std::vector<std::string>{ "assets:/fonts/a.ttf" }));
}

TEST(ArchiveRegistryTest, new_scans_reopen_same_stamp_same_size_archive_repacks)
{
	FollowupFiles fixture;
	ASSERT_TRUE(fixture.Zip({ { "folder\\a.txt", "old" } }));
	const auto files = fixture.Archive();
	const auto stamp = std::filesystem::last_write_time(fixture.root / "assets.zip");
	const auto size = std::filesystem::file_size(fixture.root / "assets.zip");
	AssetRegistry registry;
	ASSERT_TRUE(registry.Scan(files));
	ASSERT_NE(registry.Find("assets:/folder/a.txt"), nullptr);
	ASSERT_TRUE(fixture.Zip({ { "folder/b.txt", "new" } }));
	ASSERT_EQ(std::filesystem::file_size(fixture.root / "assets.zip"), size);
	std::filesystem::last_write_time(fixture.root / "assets.zip", stamp);
	ASSERT_TRUE(registry.Scan(files));
	EXPECT_EQ(registry.Find("assets:/folder/a.txt"), nullptr);
	const auto* entry = registry.Find("assets:/folder/b.txt");
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->id, 100u);
	EXPECT_EQ(entry->content_hash, ContentFingerprint("new"));
}

TEST(VirtualReferenceTest, joins_relative_dependencies_and_virtualizes_directory_sources)
{
	FollowupFiles fixture;
	fixture.Write("assets/models/a.obj", triangle);
	const auto files = fixture.Directory();
	EXPECT_EQ(files.Virtualize(fixture.root / "assets/models/a.obj"), std::optional<std::string>("assets:/models/a.obj"));
	EXPECT_EQ(VirtualFileSystem::Join("assets:/models/a.obj", "../textures/a.tga"), std::optional<std::string>("assets:/textures/a.tga"));
	EXPECT_FALSE(VirtualFileSystem::Join("assets:/a.obj", "../../outside"));
	EXPECT_FALSE(files.Virtualize(fixture.root / "outside.txt"));
}

TEST(ArchiveRegistryTest, uses_bundled_identities_without_mutating_the_archive)
{
	FollowupFiles fixture;
	ASSERT_TRUE(fixture.Zip({ { "a.txt", "source" } }));
	const auto before = std::filesystem::last_write_time(fixture.root / "assets.zip");
	AssetRegistry registry;
	ASSERT_TRUE(registry.Scan(fixture.Archive()));
	const auto* entry = registry.Find("assets:/a.txt");
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->id, 100u);
	EXPECT_TRUE(entry->source_path.empty());
	EXPECT_EQ(entry->content_hash, ContentFingerprint("source"));
	EXPECT_EQ(before, std::filesystem::last_write_time(fixture.root / "assets.zip"));
}

TEST(ArchiveRegistryTest, reports_missing_or_duplicate_bundled_metadata)
{
	FollowupFiles fixture;
	ASSERT_TRUE(fixture.Zip({ { "a.txt", "a" } }, false));
	AssetRegistry registry;
	EXPECT_FALSE(registry.Scan(fixture.Archive()));
	EXPECT_TRUE(registry.GetAssets().empty());
	ASSERT_TRUE(fixture.Zip({ { "a.txt", "a" }, { "b.txt", "b" }, { "a.txt.meta", "DaedalusAsset 1\n42\n" }, { "b.txt.meta", "DaedalusAsset 1\n42\n" } }, false));
	EXPECT_FALSE(registry.Scan(fixture.Archive()));
	EXPECT_EQ(registry.Find(42), nullptr);
	EXPECT_TRUE(registry.GetAssets().empty());
}

TEST(ContentPollingTest, detects_directory_edits_preserving_timestamp_and_size)
{
	FollowupFiles fixture;
	fixture.Write("assets/a.txt", "old");
	JobSystem jobs(1);
	AssetPipeline pipeline(jobs, fixture.Directory());
	ASSERT_TRUE(pipeline.Initialize());
	const auto time = std::filesystem::last_write_time(fixture.root / "assets/a.txt");
	fixture.Write("assets/a.txt", "new");
	std::filesystem::last_write_time(fixture.root / "assets/a.txt", time);
	std::vector<std::string> changed;
	pipeline.SetChangeCallback([&](const auto& paths) { changed = paths; });
	ASSERT_TRUE(jobs.Wait(pipeline.Refresh()));
	EXPECT_EQ(changed, (std::vector<std::string>{ "assets:/a.txt" }));
}

TEST(ContentPollingTest, detects_repacked_zip_edits_preserving_container_timestamp_and_size)
{
	FollowupFiles fixture;
	ASSERT_TRUE(fixture.Zip({ { "a.txt", "old" } }));
	JobSystem jobs(1);
	AssetPipeline pipeline(jobs, fixture.Archive());
	ASSERT_TRUE(pipeline.Initialize());
	const auto time = std::filesystem::last_write_time(fixture.root / "assets.zip");
	const auto size = std::filesystem::file_size(fixture.root / "assets.zip");
	ASSERT_TRUE(fixture.Zip({ { "a.txt", "new" } }));
	ASSERT_EQ(std::filesystem::file_size(fixture.root / "assets.zip"), size);
	std::filesystem::last_write_time(fixture.root / "assets.zip", time);
	std::vector<std::string> changed;
	pipeline.SetChangeCallback([&](const auto& paths) { changed = paths; });
	ASSERT_TRUE(jobs.Wait(pipeline.Refresh()));
	EXPECT_EQ(changed, (std::vector<std::string>{ "assets:/a.txt" }));
}

TEST(ArchivePipelineTest, reads_virtual_paths_and_uuid_on_the_owner_thread)
{
	FollowupFiles fixture;
	ASSERT_TRUE(fixture.Zip({ { "a.txt", "source" } }));
	JobSystem jobs(2);
	AssetPipeline pipeline(jobs, fixture.Archive());
	ASSERT_TRUE(pipeline.Initialize());
	const auto owner = std::this_thread::get_id();
	for (bool by_id : { false, true })
	{
		bool called = false;
		auto callback = [&](AssetLoadResult result)
		{
			called = true; EXPECT_EQ(std::this_thread::get_id(), owner);
			EXPECT_TRUE(result.Succeeded()); EXPECT_EQ(result.id, 100u); EXPECT_EQ(result.data, "source");
		};
		const auto job = by_id ? pipeline.LoadAsync(AssetId(100), callback) : pipeline.LoadAsync("assets:/a.txt", callback);
		EXPECT_TRUE(jobs.Wait(job)); EXPECT_TRUE(called);
	}
}

TEST(ArchivePipelineTest, expands_shader_includes_from_zip_entries)
{
	FollowupFiles fixture;
	ASSERT_TRUE(fixture.Zip({ { "test.glsl", "#vertex\n#include \"shared.hglsl\"\n" }, { "shared.hglsl", "float shared;\n" } }));
	std::string source, error;
	EXPECT_TRUE(AssetSourceReader::ReadShaderSource(fixture.Archive("shaders"), "shaders:/test.glsl", source, error)) << error;
	EXPECT_EQ(source, "#vertex\nfloat shared;\n");
}

TEST(TypedAssetImportTest, decodes_archive_texture_pixels_without_graphics)
{
	FollowupFiles fixture;
	ASSERT_TRUE(fixture.Zip({ { "pixel.tga", Pixel(12, 34, 56) } }));
	std::string error;
	const auto image = AssetImporter::ImportTexture(fixture.Archive(), "assets:/pixel.tga", error);
	ASSERT_NE(image, nullptr) << error;
	EXPECT_EQ(image->GetWidth(), 1); EXPECT_EQ(image->GetHeight(), 1); EXPECT_EQ(image->GetChannels(), 3);
	EXPECT_EQ(image->GetPixels()[0], 12); EXPECT_EQ(image->GetPixels()[1], 34); EXPECT_EQ(image->GetPixels()[2], 56);
}

TEST(TypedAssetImportTest, imports_directory_triangle_geometry_without_graphics)
{
	FollowupFiles fixture;
	fixture.Write("assets/triangle.obj", triangle);
	ImportedModel model;
	std::string error;
	ASSERT_TRUE(AssetImporter::ImportModel(fixture.Directory(), "assets:/triangle.obj", model, error)) << error;
	ASSERT_EQ(model.meshes.size(), 1u);
	EXPECT_EQ(model.meshes[0].vertices.size(), 3u); EXPECT_EQ(model.meshes[0].indices.size(), 3u);
	EXPECT_NE(std::find(model.dependencies.begin(), model.dependencies.end(), "assets:/triangle.obj"), model.dependencies.end());
}

TEST(TypedAssetImportTest, resolves_zip_model_materials_and_parent_relative_texture_dependencies)
{
	FollowupFiles fixture;
	const auto obj = "mtllib triangle.mtl\nusemtl Painted\n" + triangle;
	ASSERT_TRUE(fixture.Zip({ { "models/triangle.obj", obj }, { "models/triangle.mtl", "newmtl Painted\nKd 0.2 0.3 0.4\nmap_Kd ../textures/pixel.tga\n" }, { "textures/pixel.tga", Pixel(1, 2, 3) } }));
	ImportedModel model;
	std::string error;
	ASSERT_TRUE(AssetImporter::ImportModel(fixture.Archive(), "assets:/models/triangle.obj", model, error)) << error;
	for (const auto* path : { "assets:/models/triangle.obj", "assets:/models/triangle.mtl", "assets:/textures/pixel.tga" })
		EXPECT_NE(std::find(model.dependencies.begin(), model.dependencies.end(), path), model.dependencies.end()) << path;
	bool texture = false;
	for (const auto& material : model.materials) for (const auto& binding : material.textures)
		if (binding.virtual_path == "assets:/textures/pixel.tga") texture = binding.image != nullptr;
	EXPECT_TRUE(texture);
}

TEST(TypedAssetImportTest, leaves_previous_cpu_model_untouched_on_import_failure)
{
	FollowupFiles fixture;
	fixture.Write("assets/invalid.obj", "not geometry");
	ImportedModel model;
	model.dependencies = { "sentinel" };
	std::string error;
	EXPECT_FALSE(AssetImporter::ImportModel(fixture.Directory(), "assets:/invalid.obj", model, error));
	EXPECT_FALSE(error.empty()); EXPECT_EQ(model.dependencies, (std::vector<std::string>{ "sentinel" }));
}

TEST(TypedAssetImportTest, resolves_windows_zip_model_material_and_texture_names)
{
	FollowupFiles fixture;
	ASSERT_TRUE(fixture.Zip({ { "models\\triangle.obj", "mtllib triangle.mtl\nusemtl Painted\n" + triangle },
		{ "models\\triangle.mtl", "newmtl Painted\nmap_Kd ../textures/pixel.tga\n" }, { "textures\\pixel.tga", Pixel(1, 2, 3) } }));
	ImportedModel model;
	std::string error;
	ASSERT_TRUE(AssetImporter::ImportModel(fixture.Archive(), "assets:/models/triangle.obj", model, error)) << error;
	bool texture = false;
	for (const auto& material : model.materials) for (const auto& binding : material.textures)
		if (binding.virtual_path == "assets:/textures/pixel.tga") texture = binding.image != nullptr;
	EXPECT_TRUE(texture);
}

TEST(TypedAssetImportTest, rejects_texture_dependencies_outside_the_mount)
{
	FollowupFiles fixture;
	fixture.Write("assets/triangle.obj", "mtllib triangle.mtl\nusemtl Painted\n" + triangle);
	fixture.Write("assets/triangle.mtl", "newmtl Painted\nmap_Kd ../outside.tga\n");
	ImportedModel model;
	std::string error;
	EXPECT_FALSE(AssetImporter::ImportModel(fixture.Directory(), "assets:/triangle.obj", model, error));
	EXPECT_FALSE(error.empty());
}

TEST(TypedAssetImportTest, worker_import_completion_can_depend_on_owner_thread_affinity)
{
	FollowupFiles fixture;
	fixture.Write("assets/triangle.obj", triangle);
	JobSystem jobs(2);
	ImportedModel model;
	std::string error;
	bool succeeded = false, completed = false;
	const auto worker = jobs.Submit([&] { succeeded = AssetImporter::ImportModel(fixture.Directory(), "assets:/triangle.obj", model, error); });
	const auto owner = std::this_thread::get_id();
	const std::array dependencies{ worker };
	const auto continuation = jobs.Submit([&]
	{
		completed = true; EXPECT_EQ(std::this_thread::get_id(), owner); EXPECT_TRUE(succeeded) << error; EXPECT_FALSE(model.meshes.empty());
	}, dependencies, JobAffinity::MainThread);
	EXPECT_TRUE(jobs.Wait(continuation)); EXPECT_TRUE(completed);
}

TEST(VirtualSceneAssetTest, loads_an_archived_scene_by_uuid_without_graphics)
{
	FollowupFiles fixture;
	SceneRecord record;
	record.name = "Archived Scene";
	EntityRecord entity; entity.id = "42"; entity.name = "Entity";
	record.entities.push_back(entity);
	std::string json, error;
	ASSERT_TRUE(SceneJson::Serialize(record, json, error));
	ASSERT_TRUE(fixture.Zip({ { "scene.scene.json", json } }));
	JobSystem jobs(1);
	AssetPipeline pipeline(jobs, fixture.Archive());
	ASSERT_TRUE(pipeline.Initialize());
	Scene scene;
	ScriptRegistry scripts;
	EXPECT_TRUE(SceneSerializer::LoadAsset(pipeline, AssetId(100), scene, scripts, error)) << error;
	EXPECT_EQ(scene.GetSceneName(), "Archived Scene");
}

TEST(VirtualSceneAssetTest, accepts_canonical_virtual_resource_references_but_rejects_drive_paths)
{
	SceneRecord record; record.name = "Virtual";
	EntityRecord entity; entity.id = "42"; entity.model = ModelRecord{ "assets:/models/triangle.obj", 0 };
	record.entities.push_back(entity);
	std::string error;
	EXPECT_TRUE(SceneJson::Validate(record, error)) << error;
	record.entities.front().model->path = "C:/models/triangle.obj";
	EXPECT_FALSE(SceneJson::Validate(record, error));
	record.entities.front().model->path = "assets:/../outside.obj";
	EXPECT_FALSE(SceneJson::Validate(record, error));
}

TEST(VirtualSceneAssetTest, loads_windows_zip_scene_by_path_and_bundled_uuid)
{
	FollowupFiles fixture;
	SceneRecord record;
	record.name = "Windows ZIP Scene";
	std::string json, error;
	ASSERT_TRUE(SceneJson::Serialize(record, json, error));
	ASSERT_TRUE(fixture.Zip({ { "scenes\\startup.scene.json", json } }));
	JobSystem jobs(1);
	AssetPipeline pipeline(jobs, fixture.Archive());
	ASSERT_TRUE(pipeline.Initialize());
	Scene scene;
	ScriptRegistry scripts;
	EXPECT_TRUE(SceneSerializer::LoadAsset(pipeline, "assets:/scenes/startup.scene.json", scene, scripts, error)) << error;
	EXPECT_EQ(scene.GetSceneName(), record.name);
	EXPECT_TRUE(SceneSerializer::LoadAsset(pipeline, AssetId(100), scene, scripts, error)) << error;
	EXPECT_EQ(scene.GetSceneName(), record.name);
}

TEST(VirtualSceneAssetTest, reports_unknown_uuid_without_changing_scene_state)
{
	FollowupFiles fixture;
	JobSystem jobs(1); AssetPipeline pipeline(jobs, fixture.Directory()); ASSERT_TRUE(pipeline.Initialize());
	Scene scene; scene.SetSceneName("Original"); ScriptRegistry scripts; std::string error;
	EXPECT_FALSE(SceneSerializer::LoadAsset(pipeline, AssetId(0), scene, scripts, error));
	EXPECT_EQ(scene.GetSceneName(), "Original"); EXPECT_FALSE(error.empty());
}

TEST(TypedResourceContractTest, reports_absent_pipeline_through_typed_callbacks_without_gpu_access)
{
	bool texture_called = false, model_called = false;
	const auto texture = ResourceManager::LoadTextureAsync("assets:/missing.tga", [&](ResourceLoadResult<Texture2D> result)
	{
		texture_called = true; EXPECT_FALSE(result.Succeeded()); EXPECT_FALSE(result.error.empty());
	});
	const auto model = ResourceManager::LoadModelAsync(AssetId(0), [&](ResourceLoadResult<Model> result)
	{
		model_called = true; EXPECT_FALSE(result.Succeeded()); EXPECT_FALSE(result.error.empty());
	});
	EXPECT_TRUE(texture.IsComplete()); EXPECT_TRUE(model.IsComplete()); EXPECT_TRUE(texture_called); EXPECT_TRUE(model_called);
}

TEST(TypedAssetImportTest, raw_embedded_pixels_flip_rows_and_validate_dimensions)
{
	DecodedImage image;
	const std::array<unsigned char, 8> pixels{ 1, 2, 3, 4, 5, 6, 7, 8 };
	ASSERT_TRUE(image.LoadRGBA(pixels, 1, 2));
	EXPECT_EQ(image.GetPixels()[0], 5); EXPECT_EQ(image.GetPixels()[4], 1);
	EXPECT_FALSE(image.LoadRGBA(pixels, 0, 2)); EXPECT_EQ(image.GetPixels(), nullptr);
}

TEST(TypedAssetImportTest, requested_rgb_channels_match_cubemap_upload_contract)
{
	FollowupFiles fixture;
	fixture.Write("assets/pixel.tga", Pixel(1, 2, 3));
	std::string error;
	const auto image = AssetImporter::ImportTexture(fixture.Directory(), "assets:/pixel.tga", error, false, 3);
	ASSERT_NE(image, nullptr); EXPECT_EQ(image->GetChannels(), 3);
}
