#include <Core/AssetPipeline.h>
#include <ECS/Entity.h>
#include <ECS/NativeScript.h>
#include <ECS/Serialization/SceneComposer.h>
#include <ECS/Serialization/SceneJson.h>
#include <ECS/Serialization/SceneSerializer.h>
#include <ECS/Serialization/ScriptRegistry.h>
#include <Utils/Utf8.h>

#include <gtest/gtest.h>
#include <zip.h>
#include <algorithm>
#include <fstream>
#include <map>
#include <memory>
#include <set>

namespace Daedalus
{
namespace
{
	SceneRecord Template()
	{
		SceneRecord record;
		record.primary_camera = "2";
		EntityRecord body;
		body.id = "1";
		body.name = "Body";
		body.transform.translation.x = 2.f;
		body.links["camera"] = "2";
		EntityRecord camera;
		camera.id = "2";
		camera.name = "Camera";
		camera.parent = "1";
		camera.transform.translation.y = 3.f;
		camera.camera.emplace();
		record.entities = { body, camera };
		return record;
	}

	SceneRecord Instance(const std::string& path = "unit.prefab.json")
	{
		SceneRecord record;
		record.name = "Composed";
		record.primary_camera = "10";
		EntityRecord anchor;
		anchor.id = "10";
		anchor.transform.translation.x = 10.f;
		anchor.prefab = PrefabRecord{ path, { { "body", "1" } } };
		record.entities.push_back(anchor);
		return record;
	}

	const EntityRecord& EntityById(const SceneRecord& record, const std::string& id)
	{
		return *std::find_if(record.entities.begin(), record.entities.end(), [&](const auto& entity) { return entity.id == id; });
	}

	class ProbeScript : public NativeScript
	{
	public:
		using NativeScript::NativeScript;
	};
}

class ScenePrefabTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		m_root = std::filesystem::path(DL_SCENE_TEST_OUTPUT) / "prefabs" / ::testing::UnitTest::GetInstance()->current_test_info()->name();
		std::error_code ec;
		std::filesystem::remove_all(m_root, ec);
		std::filesystem::create_directories(m_root, ec);
		ASSERT_FALSE(ec) << ec.message();
		ASSERT_TRUE(m_files.Mount("assets", m_root, m_error)) << m_error;
		WriteRecord("unit.prefab.json", Template());
	}

	void TearDown() override
	{
		std::error_code ec;
		std::filesystem::remove_all(m_root, ec);
	}

	void WriteText(const std::string& name, const std::string& text)
	{
		const auto path = m_root / Utf8ToPath(name);
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);
		ASSERT_FALSE(ec);
		std::ofstream output(path, std::ios::binary);
		output.write(text.data(), static_cast<std::streamsize>(text.size()));
		ASSERT_TRUE(output.good());
	}

	void WriteRecord(const std::string& name, const SceneRecord& record)
	{
		std::string text;
		ASSERT_TRUE(SceneJson::Serialize(record, text, m_error)) << m_error;
		WriteText(name, text);
	}

	bool WriteArchive(const std::map<std::string, std::string>& entries)
	{
		int error = 0;
		std::unique_ptr<zip_t, decltype(&zip_discard)> archive(zip_open((m_root / "prefabs.zip").string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error), &zip_discard);
		if (!archive) return false;
		for (const auto& [name, text] : entries)
		{
			auto* source = zip_source_buffer(archive.get(), text.data(), text.size(), 0);
			if (!source) return false;
			if (zip_file_add(archive.get(), name.c_str(), source, ZIP_FL_ENC_UTF_8) < 0)
			{
				zip_source_free(source);
				return false;
			}
		}
		auto* raw = archive.release();
		if (zip_close(raw) == 0) return true;
		zip_discard(raw);
		return false;
	}

	std::filesystem::path m_root;
	VirtualFileSystem m_files;
	std::string m_error;
};

TEST_F(ScenePrefabTest, schema_round_trip_preserves_prefab_reference_and_exports)
{
	std::string text;
	SceneRecord parsed;
	ASSERT_TRUE(SceneJson::Serialize(Instance(), text, m_error)) << m_error;
	ASSERT_TRUE(SceneJson::Parse(text, parsed, m_error)) << m_error;
	ASSERT_TRUE(parsed.entities[0].prefab);
	EXPECT_EQ(parsed.version, CurrentSceneVersion);
	EXPECT_EQ(parsed.entities[0].prefab->path, "unit.prefab.json");
	EXPECT_EQ(parsed.entities[0].prefab->exports.at("body"), "1");
}

TEST_F(ScenePrefabTest, repeated_instances_remap_ids_parents_links_and_exports_independently)
{
	auto source = Instance();
	auto second = source.entities[0];
	second.id = "20";
	source.entities.push_back(second);
	SceneRecord composed;
	ASSERT_TRUE(SceneComposer::Expand(source, m_files, composed, m_error)) << m_error;
	ASSERT_EQ(composed.entities.size(), 6u);
	std::set<std::string> ids;
	for (const auto& entity : composed.entities)
	{
		EXPECT_TRUE(ids.insert(entity.id).second);
		EXPECT_FALSE(entity.prefab);
	}
	const auto& first_anchor = EntityById(composed, "10");
	const auto& second_anchor = EntityById(composed, "20");
	EXPECT_NE(first_anchor.links.at("body"), second_anchor.links.at("body"));
	EXPECT_NE(first_anchor.links.at("camera"), second_anchor.links.at("camera"));
	const auto& first_body = EntityById(composed, first_anchor.links.at("body"));
	const auto& second_body = EntityById(composed, second_anchor.links.at("body"));
	EXPECT_EQ(first_body.parent, "10");
	EXPECT_EQ(second_body.parent, "20");
	EXPECT_EQ(first_body.links.at("camera"), first_anchor.links.at("camera"));
	EXPECT_EQ(EntityById(composed, first_anchor.links.at("camera")).parent, first_body.id);
	EXPECT_EQ(composed.primary_camera, first_anchor.links.at("camera"));
	SceneRecord again;
	std::string a, b;
	ASSERT_TRUE(SceneComposer::Expand(source, m_files, again, m_error));
	ASSERT_TRUE(SceneJson::Serialize(composed, a, m_error));
	ASSERT_TRUE(SceneJson::Serialize(again, b, m_error));
	EXPECT_EQ(a, b);
}

TEST_F(ScenePrefabTest, physical_loading_composes_transforms_and_capture_saves_expanded_entities)
{
	WriteRecord("main.scene.json", Instance());
	Scene first, second;
	ScriptRegistry scripts;
	ASSERT_TRUE(SceneSerializer::Load(m_root / "main.scene.json", first, m_root, scripts, m_error)) << m_error;
	ASSERT_NE(first.GetCamera(), nullptr);
	EXPECT_NEAR(first.GetCamera()->GetPosition().x, 12.f, 0.0001f);
	EXPECT_NEAR(first.GetCamera()->GetPosition().y, 3.f, 0.0001f);
	EXPECT_TRUE(first.GetEntityByUUID(UUID(10)).GetLinkedEntity("body"));
	SceneRecord captured;
	ASSERT_TRUE(SceneSerializer::Capture(first, m_root, scripts, captured, m_error)) << m_error;
	EXPECT_EQ(captured.version, CurrentSceneVersion);
	ASSERT_EQ(captured.entities.size(), 3u);
	for (const auto& entity : captured.entities) EXPECT_FALSE(entity.prefab);
	ASSERT_TRUE(SceneSerializer::Instantiate(captured, second, {}, scripts, m_error)) << m_error;
	EXPECT_NEAR(second.GetCamera()->GetPosition().x, 12.f, 0.0001f);
}

TEST_F(ScenePrefabTest, nested_prefabs_forward_primary_camera_and_compose_anchor_transforms)
{
	auto nested = Instance();
	nested.entities[0].id = "7";
	nested.primary_camera = "7";
	nested.entities[0].transform.translation.x = 4.f;
	WriteRecord("outer.json", nested);
	auto source = Instance("outer.json");
	source.entities[0].prefab->exports = { { "nested", "7" } };
	Scene scene;
	ScriptRegistry scripts;
	ASSERT_TRUE(SceneSerializer::Instantiate(source, scene, m_root, scripts, m_error)) << m_error;
	EXPECT_NEAR(scene.GetCamera()->GetPosition().x, 16.f, 0.0001f);
	EXPECT_NEAR(scene.GetCamera()->GetPosition().y, 3.f, 0.0001f);
	auto nested_entity = scene.GetEntityByUUID(UUID(10)).GetLinkedEntity("nested");
	ASSERT_TRUE(nested_entity);
	EXPECT_TRUE(nested_entity.GetLinkedEntity("body"));
}

TEST_F(ScenePrefabTest, directory_asset_pipeline_composes_prefabs_before_instantiation)
{
	WriteRecord("main.scene.json", Instance());
	JobSystem jobs(1);
	AssetPipeline pipeline(jobs, m_files);
	Scene scene;
	ScriptRegistry scripts;
	ASSERT_TRUE(SceneSerializer::LoadAsset(pipeline, "assets:/main.scene.json", scene, scripts, m_error)) << m_error;
	EXPECT_NEAR(scene.GetCamera()->GetPosition().x, 12.f, 0.0001f);
}

TEST_F(ScenePrefabTest, zip_asset_pipeline_reads_nested_prefab_records_without_extraction)
{
	std::string scene_text, prefab_text;
	ASSERT_TRUE(SceneJson::Serialize(Instance(), scene_text, m_error));
	ASSERT_TRUE(SceneJson::Serialize(Template(), prefab_text, m_error));
	ASSERT_TRUE(WriteArchive({ { "main.scene.json", scene_text }, { "unit.prefab.json", prefab_text } }));
	VirtualFileSystem files;
	ASSERT_TRUE(files.MountArchive("assets", m_root / "prefabs.zip", m_error)) << m_error;
	JobSystem jobs(1);
	AssetPipeline pipeline(jobs, std::move(files));
	Scene scene;
	ScriptRegistry scripts;
	ASSERT_TRUE(SceneSerializer::LoadAsset(pipeline, "assets:/main.scene.json", scene, scripts, m_error)) << m_error;
	EXPECT_NEAR(scene.GetCamera()->GetPosition().x, 12.f, 0.0001f);
}

TEST_F(ScenePrefabTest, legacy_prefab_records_run_through_all_migration_steps)
{
	SceneRecord legacy;
	legacy.version = 1;
	legacy.primary_camera = "1";
	EntityRecord camera;
	camera.id = "1";
	camera.camera.emplace();
	camera.camera->position = { 1.f, 2.f, 3.f };
	camera.camera->yaw = -90.f;
	legacy.entities.push_back(camera);
	WriteRecord("legacy.json", legacy);
	Scene scene;
	ScriptRegistry scripts;
	ASSERT_TRUE(SceneSerializer::Instantiate(Instance("legacy.json"), scene, m_root, scripts, m_error)) << m_error;
	EXPECT_NEAR(scene.GetCamera()->GetPosition().x, 11.f, 0.0001f);
	EXPECT_NEAR(scene.GetCamera()->GetPosition().y, 2.f, 0.0001f);
	EXPECT_NEAR(scene.GetCamera()->GetDirection().z, -1.f, 0.0001f);
}

TEST_F(ScenePrefabTest, script_factories_observe_remapped_links_and_primary_camera)
{
	auto prefab = Template();
	prefab.entities[0].scripts.push_back({ "Probe", {} });
	WriteRecord("unit.prefab.json", prefab);
	ScriptRegistry scripts;
	bool observed = false;
	ASSERT_TRUE(scripts.Register<ProbeScript>("Probe",
		[](const ScriptRecord&, const EntityRecord& entity, std::string&) { return entity.links.contains("camera"); },
		[&](Entity entity, const ScriptRecord&)
		{
			observed = entity.GetLinkedEntity("camera").GetComponent<CameraComponent>().camera.get() == entity.GetScene().GetCamera();
			return entity.GetComponent<NativeScriptComponent>().AddScript<ProbeScript>(entity);
		}, [](const ProbeScript&, ScriptRecord&) {}));
	Scene scene;
	ASSERT_TRUE(SceneSerializer::Instantiate(Instance(), scene, m_root, scripts, m_error)) << m_error;
	EXPECT_TRUE(observed);
}

TEST_F(ScenePrefabTest, late_script_factory_failure_rolls_back_expanded_entities_and_metadata)
{
	auto prefab = Template();
	prefab.entities[0].scripts.push_back({ "Fail", {} });
	WriteRecord("unit.prefab.json", prefab);
	ScriptRegistry scripts;
	ASSERT_TRUE(scripts.Register<ProbeScript>("Fail",
		[](const ScriptRecord&, const EntityRecord&, std::string&) { return true; },
		[](Entity, const ScriptRecord&) -> ProbeScript* { return nullptr; }, [](const ProbeScript&, ScriptRecord&) {}));
	Scene scene;
	scene.SetSceneName("Original");
	scene.SetPaused(true);
	EXPECT_FALSE(SceneSerializer::Instantiate(Instance(), scene, m_root, scripts, m_error));
	EXPECT_TRUE(scene.IsEmpty());
	EXPECT_EQ(scene.GetSceneName(), "Original");
	EXPECT_TRUE(scene.IsPaused());
}

TEST_F(ScenePrefabTest, missing_or_malformed_prefab_leaves_scene_and_destination_unchanged)
{
	WriteText("broken.json", "{broken");
	for (const auto* path : { "missing.json", "broken.json" })
	{
		SceneRecord destination;
		destination.name = "Unchanged";
		EXPECT_FALSE(SceneComposer::Expand(Instance(path), m_files, destination, m_error));
		EXPECT_EQ(destination.name, "Unchanged");
		Scene scene;
		scene.SetSceneName("Original");
		ScriptRegistry scripts;
		EXPECT_FALSE(SceneSerializer::Instantiate(Instance(path), scene, m_root, scripts, m_error));
		EXPECT_TRUE(scene.IsEmpty());
		EXPECT_EQ(scene.GetSceneName(), "Original");
	}
}

TEST_F(ScenePrefabTest, direct_and_indirect_prefab_cycles_are_rejected)
{
	WriteRecord("self.json", Instance("self.json"));
	WriteRecord("a.json", Instance("b.json"));
	WriteRecord("b.json", Instance("a.json"));
	for (const auto* path : { "self.json", "a.json" })
	{
		SceneRecord output;
		EXPECT_FALSE(SceneComposer::Expand(Instance(path), m_files, output, m_error));
		EXPECT_NE(m_error.find("cycle"), std::string::npos) << m_error;
	}
}

TEST_F(ScenePrefabTest, nested_prefab_depth_is_bounded)
{
	for (size_t index = 0; index <= SceneComposer::MaxPrefabDepth; ++index)
		WriteRecord("depth" + std::to_string(index) + ".json", Instance("depth" + std::to_string(index + 1) + ".json"));
	SceneRecord output;
	EXPECT_FALSE(SceneComposer::Expand(Instance("depth0.json"), m_files, output, m_error));
	EXPECT_NE(m_error.find("depth limit"), std::string::npos) << m_error;
}

TEST_F(ScenePrefabTest, repeated_prefab_instance_count_is_bounded)
{
	WriteRecord("empty.json", SceneRecord{});
	SceneRecord source, output;
	for (size_t index = 0; index <= SceneComposer::MaxPrefabInstances; ++index)
	{
		EntityRecord anchor;
		anchor.id = std::to_string(index + 1);
		anchor.prefab = PrefabRecord{ "empty.json", {} };
		source.entities.push_back(anchor);
	}
	EXPECT_FALSE(SceneComposer::Expand(source, m_files, output, m_error));
	EXPECT_NE(m_error.find("instance limit"), std::string::npos) << m_error;
}

TEST_F(ScenePrefabTest, aggregate_shadow_budget_is_checked_after_expansion)
{
	auto prefab = Template();
	prefab.entities[0].directional_light.emplace();
	prefab.entities[0].directional_light->cast_shadows = true;
	prefab.entities[0].directional_light->cascades = 17;
	WriteRecord("unit.prefab.json", prefab);
	auto source = Instance();
	auto second = source.entities[0];
	second.id = "20";
	source.entities.push_back(second);
	SceneRecord output;
	EXPECT_FALSE(SceneComposer::Expand(source, m_files, output, m_error));
	EXPECT_NE(m_error.find("32 shadow-map"), std::string::npos) << m_error;
}

TEST_F(ScenePrefabTest, aggregate_physics_scale_restrictions_are_checked_after_expansion)
{
	auto prefab = Template();
	prefab.entities[0].kinematic_body.emplace();
	WriteRecord("unit.prefab.json", prefab);
	auto source = Instance();
	source.entities[0].transform.scale.x = 2.f;
	SceneRecord output;
	EXPECT_FALSE(SceneComposer::Expand(source, m_files, output, m_error));
	EXPECT_NE(m_error.find("unit scale"), std::string::npos) << m_error;
}

TEST_F(ScenePrefabTest, exports_reject_missing_entities_overwrites_and_wrong_component_types)
{
	for (int mode = 0; mode < 3; ++mode)
	{
		auto source = Instance();
		if (mode == 0) source.entities[0].prefab->exports["body"] = "99";
		if (mode == 1) source.entities[0].links["body"] = "10";
		if (mode == 2) source.entities[0].prefab->exports["camera"] = "1";
		SceneRecord output;
		output.name = "Original";
		EXPECT_FALSE(SceneComposer::Expand(source, m_files, output, m_error)) << mode;
		EXPECT_EQ(output.name, "Original");
	}
}

TEST_F(ScenePrefabTest, camera_selection_requires_an_explicit_valid_scene_choice)
{
	auto source = Instance();
	source.primary_camera.clear();
	SceneRecord output;
	EXPECT_FALSE(SceneComposer::Expand(source, m_files, output, m_error));
	WriteRecord("empty.json", SceneRecord{});
	source = Instance("empty.json");
	source.entities[0].prefab->exports.clear();
	EXPECT_FALSE(SceneComposer::Expand(source, m_files, output, m_error));
	EXPECT_NE(m_error.find("no primary camera"), std::string::npos) << m_error;
}

TEST_F(ScenePrefabTest, remapping_does_not_rewrite_untyped_script_strings)
{
	auto prefab = Template();
	prefab.entities[0].scripts.push_back({ "Custom", { { "label", std::string("2") } } });
	WriteRecord("unit.prefab.json", prefab);
	auto source = Instance();
	for (const auto* id : { "1", "2" })
	{
		EntityRecord occupied;
		occupied.id = id;
		source.entities.push_back(occupied);
	}
	SceneRecord output;
	ASSERT_TRUE(SceneComposer::Expand(source, m_files, output, m_error)) << m_error;
	const auto& body = EntityById(output, output.entities[0].links.at("body"));
	EXPECT_NE(body.links.at("camera"), "2");
	EXPECT_EQ(std::get<std::string>(body.scripts[0].properties.at("label")), "2");
}

TEST_F(ScenePrefabTest, traversal_paths_and_unknown_override_fields_are_rejected)
{
	auto source = Instance("../outside.json");
	SceneRecord output;
	EXPECT_FALSE(SceneComposer::Expand(source, m_files, output, m_error));
	EXPECT_FALSE(SceneJson::Parse(R"({"version":3,"entities":[{"id":"1","prefab":{"path":"unit.prefab.json","overrides":{}}}]})", output, m_error));
}
}
