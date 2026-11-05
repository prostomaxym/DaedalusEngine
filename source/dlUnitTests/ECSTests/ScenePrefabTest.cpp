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
#include <limits>
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

TEST_F(ScenePrefabTest, override_schema_round_trips_components_and_removals_without_legacy_poses)
{
	auto source = Instance();
	EntityOverrideRecord patch;
	patch.id = "1";
	patch.name = "Overridden";
	patch.transform = TransformRecord{};
	patch.transform->translation.x = 7.f;
	patch.camera = CameraRecord{};
	patch.camera->movement_speed = 9.f;
	patch.remove = { "rigid_body" };
	source.entities[0].prefab->overrides.push_back(patch);
	std::string text;
	SceneRecord parsed;
	ASSERT_TRUE(SceneJson::Serialize(source, text, m_error)) << m_error;
	EXPECT_EQ(text.find("\"position\""), std::string::npos);
	ASSERT_TRUE(SceneJson::Parse(text, parsed, m_error)) << m_error;
	const auto& actual = parsed.entities[0].prefab->overrides[0];
	EXPECT_EQ(actual.name, "Overridden");
	EXPECT_FLOAT_EQ(actual.transform->translation.x, 7.f);
	EXPECT_FLOAT_EQ(actual.camera->movement_speed, 9.f);
	EXPECT_EQ(actual.remove, patch.remove);
}

TEST_F(ScenePrefabTest, legacy_versions_reject_variant_and_override_features)
{
	for (uint32_t version = 1; version < 4; ++version)
	{
		auto source = Instance();
		source.version = version;
		EntityOverrideRecord patch;
		patch.id = "1";
		patch.name = "New";
		source.entities[0].prefab->overrides.push_back(patch);
		EXPECT_FALSE(SceneJson::Validate(source, m_error));
		source = SceneRecord{};
		source.version = version;
		source.base_prefab = "unit.prefab.json";
		EXPECT_FALSE(SceneJson::Validate(source, m_error));
	}
}

TEST_F(ScenePrefabTest, variants_inherit_base_entities_and_instance_overrides_win)
{
	SceneRecord variant;
	variant.base_prefab = "unit.prefab.json";
	EntityOverrideRecord patch;
	patch.id = "1";
	patch.name = "Variant body";
	patch.transform = TransformRecord{};
	patch.transform->translation.x = 5.f;
	variant.overrides.push_back(patch);
	WriteRecord("variant.json", variant);
	SceneRecord derived;
	derived.base_prefab = "variant.json";
	patch.name = "Derived body";
	patch.transform.reset();
	derived.overrides.push_back(patch);
	WriteRecord("derived.json", derived);
	auto source = Instance("derived.json");
	patch.name = "Instance body";
	source.entities[0].prefab->overrides.push_back(patch);
	SceneRecord output;
	ASSERT_TRUE(SceneComposer::Expand(source, m_files, output, m_error)) << m_error;
	const auto& body = EntityById(output, output.entities[0].links.at("body"));
	EXPECT_EQ(body.name, "Instance body");
	EXPECT_FLOAT_EQ(body.transform.translation.x, 5.f);
	EXPECT_EQ(body.links.at("camera"), output.primary_camera);
	EXPECT_EQ(body.prefab_origins[0].path, "assets:/derived.json");
}

TEST_F(ScenePrefabTest, direct_variant_loading_resolves_inherited_primary_camera)
{
	SceneRecord variant;
	variant.name = "Variant scene";
	variant.base_prefab = "unit.prefab.json";
	Scene scene;
	ScriptRegistry scripts;
	ASSERT_TRUE(SceneSerializer::Instantiate(variant, scene, m_root, scripts, m_error)) << m_error;
	EXPECT_EQ(scene.GetSceneName(), "Variant scene");
	EXPECT_NE(scene.GetCamera(), nullptr);
}

TEST_F(ScenePrefabTest, repeated_instances_keep_overrides_independent)
{
	auto source = Instance();
	EntityOverrideRecord patch;
	patch.id = "1";
	patch.name = "First body";
	source.entities[0].prefab->overrides.push_back(patch);
	auto second = source.entities[0];
	second.id = "20";
	second.prefab->overrides[0].name = "Second body";
	source.entities.push_back(second);
	SceneRecord output;
	ASSERT_TRUE(SceneComposer::Expand(source, m_files, output, m_error)) << m_error;
	EXPECT_EQ(EntityById(output, EntityById(output, "10").links.at("body")).name, "First body");
	EXPECT_EQ(EntityById(output, EntityById(output, "20").links.at("body")).name, "Second body");
	EXPECT_EQ(Template().entities[0].name, "Body");
}

TEST_F(ScenePrefabTest, overrides_replace_components_scripts_links_and_parent_before_remapping)
{
	auto prefab = Template();
	prefab.entities[0].kinematic_body.emplace();
	prefab.entities[0].scripts.push_back({ "Old", {} });
	WriteRecord("unit.prefab.json", prefab);
	auto source = Instance();
	EntityOverrideRecord patch;
	patch.id = "1";
	patch.remove = { "kinematic_body" };
	patch.rigid_body = BodyRecord{};
	patch.rigid_body->mass = 2.f;
	patch.scripts = std::vector<ScriptRecord>{};
	patch.links = std::map<std::string, std::string>{ { "camera", "2" } };
	source.entities[0].prefab->overrides.push_back(patch);
	patch = EntityOverrideRecord{};
	patch.id = "2";
	patch.parent = "";
	patch.camera = CameraRecord{};
	patch.camera->fov = 90.f;
	source.entities[0].prefab->overrides.push_back(patch);
	SceneRecord output;
	ASSERT_TRUE(SceneComposer::Expand(source, m_files, output, m_error)) << m_error;
	const auto& body = EntityById(output, output.entities[0].links.at("body"));
	EXPECT_FALSE(body.kinematic_body);
	ASSERT_TRUE(body.rigid_body);
	EXPECT_FLOAT_EQ(body.rigid_body->mass, 2.f);
	EXPECT_TRUE(body.scripts.empty());
	EXPECT_EQ(body.links.at("camera"), output.primary_camera);
	EXPECT_EQ(EntityById(output, output.primary_camera).parent, "10");
	EXPECT_FLOAT_EQ(EntityById(output, output.primary_camera).camera->fov, 90.f);
}

TEST_F(ScenePrefabTest, invalid_overrides_leave_composition_output_unchanged)
{
	for (int mode = 0; mode < 9; ++mode)
	{
		auto source = Instance();
		EntityOverrideRecord patch;
		patch.id = "1";
		if (mode == 0) patch.id = "99";
		if (mode == 1) patch.parent = "2";
		if (mode == 2) patch.links = std::map<std::string, std::string>{ { "camera", "1" } };
		if (mode == 3) patch.remove = { "unknown" };
		if (mode == 4) { patch.camera = CameraRecord{}; patch.remove = { "camera" }; }
		if (mode == 5) { patch.transform = TransformRecord{}; patch.transform->scale.x = 0.f; }
		if (mode == 6) { patch.rigid_body = BodyRecord{}; patch.rigid_body->radius = -1.f; }
		if (mode == 7) { patch.camera = CameraRecord{}; patch.camera->fov = std::numeric_limits<float>::quiet_NaN(); }
		if (mode == 8) { patch.id = "2"; patch.remove = { "camera" }; }
		source.entities[0].prefab->overrides.push_back(patch);
		SceneRecord output;
		output.name = "Unchanged";
		EXPECT_FALSE(SceneComposer::Expand(source, m_files, output, m_error)) << mode;
		EXPECT_EQ(output.name, "Unchanged");
		EXPECT_FALSE(m_error.empty());
	}
}

TEST_F(ScenePrefabTest, override_schema_rejects_duplicate_targets_and_legacy_pose_fields)
{
	SceneRecord output;
	EXPECT_FALSE(SceneJson::Parse(R"({"version":4,"entities":[{"id":"10","prefab":{"path":"unit.prefab.json","overrides":[{"id":"1"},{"id":"1"}]}}]})", output, m_error));
	EXPECT_FALSE(SceneJson::Parse(R"({"version":4,"entities":[{"id":"10","prefab":{"path":"unit.prefab.json","overrides":[{"id":"2","camera":{"position":[0,0,0]}}]}}]})", output, m_error));
}

TEST_F(ScenePrefabTest, variant_schema_rejects_authored_entities_and_missing_base)
{
	auto variant = Template();
	variant.base_prefab = "unit.prefab.json";
	EXPECT_FALSE(SceneJson::Validate(variant, m_error));
	variant = SceneRecord{};
	EntityOverrideRecord patch;
	patch.id = "1";
	variant.overrides.push_back(patch);
	EXPECT_FALSE(SceneJson::Validate(variant, m_error));
}

TEST_F(ScenePrefabTest, variant_and_mixed_nested_cycles_are_rejected)
{
	SceneRecord a, b;
	a.base_prefab = "b.json";
	b.base_prefab = "a.json";
	WriteRecord("a.json", a);
	WriteRecord("b.json", b);
	SceneRecord output;
	EXPECT_FALSE(SceneComposer::Expand(Instance("a.json"), m_files, output, m_error));
	EXPECT_NE(m_error.find("cycle"), std::string::npos);
	WriteRecord("b.json", Instance("a.json"));
	EXPECT_FALSE(SceneComposer::Expand(Instance("a.json"), m_files, output, m_error));
	EXPECT_NE(m_error.find("cycle"), std::string::npos);
}

TEST_F(ScenePrefabTest, variant_depth_uses_the_nested_prefab_budget)
{
	for (size_t index = 0; index <= SceneComposer::MaxPrefabDepth; ++index)
	{
		SceneRecord variant;
		variant.base_prefab = "variant" + std::to_string(index + 1) + ".json";
		WriteRecord("variant" + std::to_string(index) + ".json", variant);
	}
	SceneRecord output;
	EXPECT_FALSE(SceneComposer::Expand(Instance("variant0.json"), m_files, output, m_error));
	EXPECT_NE(m_error.find("depth limit"), std::string::npos);
}

TEST_F(ScenePrefabTest, nested_runtime_provenance_tracks_remapped_instance_lineage)
{
	WriteRecord("outer.json", Instance());
	auto source = Instance("outer.json");
	source.entities[0].prefab->exports = { { "nested", "10" } };
	Scene scene;
	ScriptRegistry scripts;
	ASSERT_TRUE(SceneSerializer::Instantiate(source, scene, m_root, scripts, m_error)) << m_error;
	auto nested = scene.GetEntityByUUID(UUID(10)).GetLinkedEntity("nested");
	ASSERT_TRUE(nested);
	auto body = nested.GetLinkedEntity("body");
	const auto origins = scene.GetPrefabOrigins(body.GetUUID());
	ASSERT_EQ(origins.size(), 2u);
	EXPECT_EQ(origins[0].path, "assets:/outer.json");
	EXPECT_EQ(origins[0].instance, "10");
	EXPECT_EQ(origins[1].path, "assets:/unit.prefab.json");
	EXPECT_EQ(origins[1].instance, std::to_string(static_cast<uint64_t>(nested.GetUUID())));
	EXPECT_EQ(origins[1].template_entity, "1");
	const auto id = body.GetUUID();
	scene.DestroyEntity(body);
	EXPECT_TRUE(scene.GetPrefabOrigins(id).empty());
	scene.Clear();
	EXPECT_TRUE(scene.GetPrefabOrigins(UUID(10)).empty());
}

TEST_F(ScenePrefabTest, expanded_capture_does_not_restore_prefab_provenance)
{
	Scene scene, restored;
	ScriptRegistry scripts;
	ASSERT_TRUE(SceneSerializer::Instantiate(Instance(), scene, m_root, scripts, m_error)) << m_error;
	SceneRecord captured;
	ASSERT_TRUE(SceneSerializer::Capture(scene, m_root, scripts, captured, m_error)) << m_error;
	for (const auto& entity : captured.entities)
	{
		EXPECT_FALSE(entity.prefab);
		EXPECT_TRUE(entity.prefab_origins.empty());
	}
	ASSERT_TRUE(SceneSerializer::Instantiate(captured, restored, {}, scripts, m_error)) << m_error;
	EXPECT_TRUE(restored.GetPrefabOrigins(UUID(10)).empty());
}

TEST_F(ScenePrefabTest, repeated_spawning_preserves_existing_entities_camera_and_metadata)
{
	Scene scene;
	ScriptRegistry scripts;
	ASSERT_TRUE(SceneSerializer::Instantiate(Template(), scene, {}, scripts, m_error)) << m_error;
	scene.SetSceneName("Existing");
	scene.SetPaused(true);
	const auto* camera = scene.GetCamera();
	UUID first(0), second(0);
	const auto prefab = *Instance().entities[0].prefab;
	ASSERT_TRUE(SceneSerializer::SpawnPrefab(prefab, scene, m_root, scripts, first, m_error)) << m_error;
	ASSERT_TRUE(SceneSerializer::SpawnPrefab(prefab, scene, m_root, scripts, second, m_error)) << m_error;
	EXPECT_NE(static_cast<uint64_t>(first), static_cast<uint64_t>(second));
	EXPECT_TRUE(scene.GetEntityByUUID(UUID(1)));
	EXPECT_TRUE(scene.GetEntityByUUID(UUID(2)));
	EXPECT_EQ(scene.GetCamera(), camera);
	EXPECT_EQ(scene.GetSceneName(), "Existing");
	EXPECT_TRUE(scene.IsPaused());
	auto first_body = scene.GetEntityByUUID(first).GetLinkedEntity("body");
	auto second_body = scene.GetEntityByUUID(second).GetLinkedEntity("body");
	ASSERT_TRUE(first_body);
	ASSERT_TRUE(second_body);
	EXPECT_NE(first_body.GetUUID(), second_body.GetUUID());
	EXPECT_EQ(scene.GetPrefabOrigins(first_body.GetUUID())[0].instance, std::to_string(static_cast<uint64_t>(first)));
}

TEST_F(ScenePrefabTest, camera_free_prefabs_spawn_without_selecting_a_primary_camera)
{
	SceneRecord prefab;
	EntityRecord body;
	body.id = "1";
	prefab.entities.push_back(body);
	WriteRecord("body.json", prefab);
	Scene scene;
	ScriptRegistry scripts;
	UUID instance(0);
	ASSERT_TRUE(SceneSerializer::SpawnPrefab({ "body.json", {} }, scene, m_root, scripts, instance, m_error)) << m_error;
	EXPECT_NE(static_cast<uint64_t>(instance), 0u);
	EXPECT_EQ(scene.GetCamera(), nullptr);
}

TEST_F(ScenePrefabTest, spawning_a_camera_requires_explicit_selection_before_capture)
{
	Scene scene;
	ScriptRegistry scripts;
	UUID instance(0);
	ASSERT_TRUE(SceneSerializer::SpawnPrefab(*Instance().entities[0].prefab, scene, m_root, scripts, instance, m_error)) << m_error;
	SceneRecord captured;
	EXPECT_FALSE(SceneSerializer::Capture(scene, m_root, scripts, captured, m_error));
	auto camera = scene.GetEntityByUUID(instance).GetLinkedEntity("camera");
	ASSERT_TRUE(camera);
	ASSERT_TRUE(scene.SetPrimaryCamera(camera.GetUUID()));
	ASSERT_TRUE(SceneSerializer::Capture(scene, m_root, scripts, captured, m_error)) << m_error;
	EXPECT_EQ(captured.primary_camera, std::to_string(static_cast<uint64_t>(camera.GetUUID())));
}

TEST_F(ScenePrefabTest, spawned_parented_bodies_have_world_poses_before_script_construction)
{
	auto prefab = Template();
	prefab.entities[0].rigid_body.emplace();
	prefab.entities[0].rigid_body->mass = 1.f;
	prefab.entities[0].scripts.push_back({ "Probe", {} });
	WriteRecord("unit.prefab.json", prefab);
	Scene scene;
	auto parent = scene.CreateEntity("Parent");
	parent.GetComponent<TransformComponent>().translation.x = 20.f;
	ScriptRegistry scripts;
	bool observed = false;
	ASSERT_TRUE(scripts.Register<ProbeScript>("Probe",
		[](const ScriptRecord&, const EntityRecord&, std::string&) { return true; },
		[&](Entity entity, const ScriptRecord&)
		{
			glm::vec3 position;
			glm::quat orientation;
			const auto handle = entity.GetComponent<RigidBodyComponent>().handle;
			observed = scene.GetPhysicsWorld().GetRigidBodyTransform(handle, position, orientation);
			EXPECT_NEAR(position.x, 32.f, 0.0001f);
			EXPECT_FALSE(scene.GetPrefabOrigins(entity.GetUUID()).empty());
			return entity.GetComponent<NativeScriptComponent>().AddScript<ProbeScript>(entity);
		}, [](const ProbeScript&, ScriptRecord&) {}));
	TransformRecord transform;
	transform.translation.x = 10.f;
	UUID instance(0);
	ASSERT_TRUE(SceneSerializer::SpawnPrefab(*Instance().entities[0].prefab, scene, m_root, scripts, instance, m_error, transform, parent.GetUUID())) << m_error;
	EXPECT_TRUE(observed);
	EXPECT_EQ(scene.GetParent(instance), parent.GetUUID());
}

TEST_F(ScenePrefabTest, loaded_stopped_bodies_initialize_after_prestart_transform_edits)
{
	auto prefab = Template();
	prefab.entities[0].rigid_body.emplace();
	prefab.entities[0].rigid_body->mass = 1.f;
	WriteRecord("unit.prefab.json", prefab);
	Scene scene;
	ScriptRegistry scripts;
	ASSERT_TRUE(SceneSerializer::Instantiate(Instance(), scene, m_root, scripts, m_error)) << m_error;
	auto anchor = scene.GetEntityByUUID(UUID(10));
	auto body = anchor.GetLinkedEntity("body");
	EXPECT_FALSE(body.GetComponent<RigidBodyComponent>().pose_initialized);
	anchor.GetComponent<TransformComponent>().translation.x = 20.f;
	scene.StepPhysics(DeltaTime(std::chrono::nanoseconds(16666667)));
	glm::vec3 position;
	glm::quat orientation;
	ASSERT_TRUE(scene.GetPhysicsWorld().GetRigidBodyTransform(body.GetComponent<RigidBodyComponent>().handle, position, orientation));
	EXPECT_NEAR(position.x, 22.f, 0.0001f);
}

TEST_F(ScenePrefabTest, spawned_stopped_bodies_honor_transform_edits_before_first_step)
{
	auto prefab = Template();
	prefab.entities[0].rigid_body.emplace();
	prefab.entities[0].rigid_body->mass = 1.f;
	WriteRecord("unit.prefab.json", prefab);
	Scene scene;
	ScriptRegistry scripts;
	UUID instance(0);
	ASSERT_TRUE(SceneSerializer::SpawnPrefab(*Instance().entities[0].prefab, scene, m_root, scripts, instance, m_error)) << m_error;
	auto anchor = scene.GetEntityByUUID(instance);
	auto body = anchor.GetLinkedEntity("body");
	EXPECT_FALSE(body.GetComponent<RigidBodyComponent>().pose_initialized);
	anchor.GetComponent<TransformComponent>().translation.x = 20.f;
	scene.StepPhysics(DeltaTime(std::chrono::nanoseconds(16666667)));
	glm::vec3 position;
	glm::quat orientation;
	ASSERT_TRUE(scene.GetPhysicsWorld().GetRigidBodyTransform(body.GetComponent<RigidBodyComponent>().handle, position, orientation));
	EXPECT_NEAR(position.x, 22.f, 0.0001f);
}

TEST_F(ScenePrefabTest, late_spawn_failure_preserves_existing_scene_and_releases_spawned_bodies)
{
	auto prefab = Template();
	prefab.entities[0].rigid_body.emplace();
	prefab.entities[0].scripts.push_back({ "Fail", {} });
	WriteRecord("unit.prefab.json", prefab);
	Scene scene;
	ScriptRegistry scripts;
	ASSERT_TRUE(SceneSerializer::Instantiate(Template(), scene, {}, scripts, m_error)) << m_error;
	scene.SetPaused(true);
	SceneRecord before, after;
	ASSERT_TRUE(SceneSerializer::Capture(scene, {}, scripts, before, m_error));
	RigidBodyHandle spawned_body = 0;
	ASSERT_TRUE(scripts.Register<ProbeScript>("Fail",
		[](const ScriptRecord&, const EntityRecord&, std::string&) { return true; },
		[&](Entity entity, const ScriptRecord&) -> ProbeScript*
		{
			spawned_body = entity.GetComponent<RigidBodyComponent>().handle;
			return nullptr;
		}, [](const ProbeScript&, ScriptRecord&) {}));
	UUID instance(123);
	const auto* camera = scene.GetCamera();
	EXPECT_FALSE(SceneSerializer::SpawnPrefab(*Instance().entities[0].prefab, scene, m_root, scripts, instance, m_error));
	EXPECT_EQ(static_cast<uint64_t>(instance), 123u);
	EXPECT_EQ(scene.GetCamera(), camera);
	EXPECT_TRUE(scene.IsPaused());
	ASSERT_NE(spawned_body, 0u);
	glm::vec3 position;
	glm::quat orientation;
	EXPECT_FALSE(scene.GetPhysicsWorld().GetRigidBodyTransform(spawned_body, position, orientation));
	ASSERT_TRUE(SceneSerializer::Capture(scene, {}, scripts, after, m_error));
	std::string a, b;
	ASSERT_TRUE(SceneJson::Serialize(before, a, m_error));
	ASSERT_TRUE(SceneJson::Serialize(after, b, m_error));
	EXPECT_EQ(a, b);
}

TEST_F(ScenePrefabTest, script_validation_failure_does_not_append_any_entities)
{
	auto prefab = Template();
	prefab.entities[0].scripts.push_back({ "Unknown", {} });
	WriteRecord("unit.prefab.json", prefab);
	Scene scene;
	ScriptRegistry scripts;
	UUID instance(7);
	EXPECT_FALSE(SceneSerializer::SpawnPrefab(*Instance().entities[0].prefab, scene, m_root, scripts, instance, m_error));
	EXPECT_TRUE(scene.IsEmpty());
	EXPECT_EQ(static_cast<uint64_t>(instance), 7u);
}

TEST_F(ScenePrefabTest, reentrant_spawning_from_a_factory_is_rejected)
{
	auto prefab = Template();
	prefab.entities[0].scripts.push_back({ "Probe", {} });
	WriteRecord("unit.prefab.json", prefab);
	Scene scene;
	ScriptRegistry scripts;
	bool rejected = false;
	ASSERT_TRUE(scripts.Register<ProbeScript>("Probe",
		[](const ScriptRecord&, const EntityRecord&, std::string&) { return true; },
		[&](Entity entity, const ScriptRecord&)
		{
			UUID nested(0);
			std::string error;
			rejected = !SceneSerializer::SpawnPrefab(*Instance().entities[0].prefab, scene, m_root, scripts, nested, error);
			EXPECT_NE(error.find("boundary"), std::string::npos);
			return entity.GetComponent<NativeScriptComponent>().AddScript<ProbeScript>(entity);
		}, [](const ProbeScript&, ScriptRecord&) {}));
	UUID instance(0);
	ASSERT_TRUE(SceneSerializer::SpawnPrefab(*Instance().entities[0].prefab, scene, m_root, scripts, instance, m_error)) << m_error;
	EXPECT_TRUE(rejected);
}

TEST_F(ScenePrefabTest, spawning_checks_existing_ancestor_scale_and_unknown_parent)
{
	auto prefab = Template();
	prefab.entities[0].kinematic_body.emplace();
	WriteRecord("unit.prefab.json", prefab);
	Scene scene;
	auto parent = scene.CreateEntity("Parent");
	parent.GetComponent<TransformComponent>().scale.x = 2.f;
	ScriptRegistry scripts;
	UUID instance(0);
	EXPECT_FALSE(SceneSerializer::SpawnPrefab(*Instance().entities[0].prefab, scene, m_root, scripts, instance, m_error, {}, parent.GetUUID()));
	EXPECT_NE(m_error.find("unit scale"), std::string::npos);
	EXPECT_FALSE(SceneSerializer::SpawnPrefab(*Instance().entities[0].prefab, scene, m_root, scripts, instance, m_error, {}, UUID(0xffff)));
	SceneRecord captured;
	ASSERT_TRUE(SceneSerializer::Capture(scene, {}, scripts, captured, m_error));
	EXPECT_EQ(captured.entities.size(), 1u);
}

TEST_F(ScenePrefabTest, spawning_checks_the_existing_scene_shadow_budget)
{
	auto prefab = Template();
	prefab.entities[0].directional_light.emplace();
	prefab.entities[0].directional_light->cast_shadows = true;
	prefab.entities[0].directional_light->cascades = 17;
	WriteRecord("unit.prefab.json", prefab);
	Scene scene;
	ScriptRegistry scripts;
	ASSERT_TRUE(SceneSerializer::Instantiate(prefab, scene, {}, scripts, m_error)) << m_error;
	UUID instance(0);
	EXPECT_FALSE(SceneSerializer::SpawnPrefab(*Instance().entities[0].prefab, scene, m_root, scripts, instance, m_error));
	EXPECT_NE(m_error.find("32 shadow-map"), std::string::npos);
}

TEST_F(ScenePrefabTest, spawning_during_physics_iteration_is_rejected)
{
	Scene scene;
	const auto first = scene.GetPhysicsWorld().CreateBoxRigidBody(0.f, glm::vec3(1.f), glm::vec3(0.f));
	const auto second = scene.GetPhysicsWorld().CreateBoxRigidBody(1.f, glm::vec3(1.f), glm::vec3(0.f));
	ASSERT_NE(first, 0u);
	ASSERT_NE(second, 0u);
	ScriptRegistry scripts;
	bool called = false;
	scene.GetPhysicsWorld().RegisterCollisionCallback([&](RigidBodyHandle, RigidBodyHandle)
	{
		called = true;
		UUID instance(0);
		EXPECT_FALSE(SceneSerializer::SpawnPrefab(*Instance().entities[0].prefab, scene, m_root, scripts, instance, m_error));
		EXPECT_NE(m_error.find("boundary"), std::string::npos);
	});
	scene.StepPhysics(DeltaTime(std::chrono::nanoseconds(16666667)));
	EXPECT_TRUE(called);
	EXPECT_TRUE(scene.IsEmpty());
}

TEST_F(ScenePrefabTest, zip_pipeline_spawns_variants_with_instance_overrides)
{
	SceneRecord variant;
	variant.base_prefab = "unit.prefab.json";
	std::string variant_text, prefab_text;
	ASSERT_TRUE(SceneJson::Serialize(variant, variant_text, m_error));
	ASSERT_TRUE(SceneJson::Serialize(Template(), prefab_text, m_error));
	ASSERT_TRUE(WriteArchive({ { "variant.json", variant_text }, { "unit.prefab.json", prefab_text } }));
	VirtualFileSystem files;
	ASSERT_TRUE(files.MountArchive("assets", m_root / "prefabs.zip", m_error)) << m_error;
	JobSystem jobs(1);
	AssetPipeline pipeline(jobs, std::move(files));
	auto prefab = *Instance("variant.json").entities[0].prefab;
	EntityOverrideRecord patch;
	patch.id = "1";
	patch.name = "ZIP instance";
	prefab.overrides.push_back(patch);
	Scene scene;
	ScriptRegistry scripts;
	UUID instance(0);
	ASSERT_TRUE(SceneSerializer::SpawnPrefab(prefab, scene, {}, scripts, instance, m_error, {}, UUID(0), &pipeline)) << m_error;
	EXPECT_EQ(scene.GetEntityByUUID(instance).GetLinkedEntity("body").GetName(), "ZIP instance");
}
}
