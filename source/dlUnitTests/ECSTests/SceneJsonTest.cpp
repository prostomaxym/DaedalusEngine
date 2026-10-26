#include <ECS/Serialization/SceneJson.h>

#include <gtest/gtest.h>
#include <limits>

using namespace Daedalus;

namespace
{
	SceneRecord Sample()
	{
		SceneRecord record;
		record.name = "Round trip";
		record.primary_camera = "18446744073709551615";
		EntityRecord entity;
		entity.id = record.primary_camera;
		entity.name = "Camera";
		entity.camera.emplace();
		entity.transform.translation = { 1.f, 2.f, 3.f };
		entity.transform.rotation = { 10.f, 20.f, 30.f };
		entity.model = ModelRecord{ "models/example.obj", 6291659 };
		entity.directional_light.emplace();
		entity.point_light.emplace();
		entity.spot_light.emplace();
		entity.skybox = SkyboxRecord{ { "sky/r.png", "sky/l.png", "sky/t.png", "sky/b.png", "sky/f.png", "sky/k.png" }, 15.f };
		entity.rigid_body.emplace();
		entity.rigid_body->mass = 42.f;
		entity.rigid_body->position = { 9.f, 8.f, 7.f };
		entity.scripts.push_back({ "Example", { { "speed", 2.5 }, { "enabled", true }, { "label", std::string("test") }, { "direction", glm::vec3(1.f, 0.f, 0.f) } } });
		record.entities.push_back(std::move(entity));
		return record;
	}
}

TEST(SceneJsonTest, point_light_does_not_require_a_direction)
{
	auto record = Sample();
	record.entities[0].point_light->direction = glm::vec3(0.f);
	std::string error;
	EXPECT_TRUE(SceneJson::Validate(record, error)) << error;
}

TEST(SceneJsonTest, shadow_budget_counts_point_light_faces)
{
	auto record = Sample();
	auto& entity = record.entities[0];
	entity.directional_light->cast_shadows = true;
	entity.directional_light->cascades = 26;
	entity.point_light->cast_shadows = true;
	std::string error;
	EXPECT_TRUE(SceneJson::Validate(record, error)) << error;
	entity.directional_light->cascades = 27;
	EXPECT_FALSE(SceneJson::Validate(record, error));
	EXPECT_NE(error.find("32 shadow-map"), std::string::npos);
}

TEST(SceneJsonTest, rejects_nonpositive_collider_dimensions)
{
	auto record = Sample();
	record.entities[0].rigid_body->half_extents.x = 0.f;
	std::string error;
	EXPECT_FALSE(SceneJson::Validate(record, error));
}

TEST(SceneJsonTest, rejects_multiple_spotlight_cascades_before_resource_construction)
{
	auto record = Sample();
	record.entities[0].spot_light->cascades = 2;
	std::string error;
	EXPECT_FALSE(SceneJson::Validate(record, error));
	EXPECT_NE(error.find("exactly one cascade"), std::string::npos);
}

TEST(SceneJsonTest, round_trip_preserves_component_configuration)
{
	const auto input = Sample();
	SceneRecord output;
	std::string text, error, second;
	ASSERT_TRUE(SceneJson::Serialize(input, text, error)) << error;
	ASSERT_TRUE(SceneJson::Parse(text, output, error)) << error;
	ASSERT_EQ(output.entities.size(), 1u);
	const auto& entity = output.entities.front();
	EXPECT_EQ(entity.id, "18446744073709551615");
	EXPECT_EQ(output.primary_camera, entity.id);
	EXPECT_FLOAT_EQ(entity.transform.rotation.y, 20.f);
	ASSERT_TRUE(entity.rigid_body);
	EXPECT_FLOAT_EQ(entity.rigid_body->mass, 42.f);
	EXPECT_FLOAT_EQ(entity.rigid_body->position.x, 9.f);
	ASSERT_TRUE(entity.skybox);
	EXPECT_EQ(entity.skybox->faces.size(), 6u);
	EXPECT_DOUBLE_EQ(std::get<double>(entity.scripts[0].properties.at("speed")), 2.5);
	ASSERT_TRUE(SceneJson::Serialize(output, second, error)) << error;
	EXPECT_EQ(text, second);
}

TEST(SceneJsonTest, defaults_fill_omitted_optional_properties)
{
	SceneRecord record;
	std::string error;
	ASSERT_TRUE(SceneJson::Parse(R"({"version":1,"entities":[{"id":"1"}]})", record, error)) << error;
	EXPECT_EQ(record.entities[0].name, "Entity");
	EXPECT_FLOAT_EQ(record.entities[0].transform.scale.x, 1.f);
	EXPECT_FALSE(record.entities[0].model);
}

TEST(SceneJsonTest, parse_failure_does_not_change_destination)
{
	SceneRecord record = Sample();
	std::string error;
	EXPECT_FALSE(SceneJson::Parse("{broken", record, error));
	EXPECT_EQ(record.name, "Round trip");
	EXPECT_FALSE(error.empty());
}

TEST(SceneJsonTest, rejects_duplicate_json_keys)
{
	SceneRecord record;
	std::string error;
	EXPECT_FALSE(SceneJson::Parse(R"({"version":1,"version":1,"entities":[]})", record, error));
	EXPECT_NE(error.find("duplicate"), std::string::npos);
}

TEST(SceneJsonTest, rejects_unknown_components)
{
	SceneRecord record;
	std::string error;
	EXPECT_FALSE(SceneJson::Parse(R"({"version":1,"entities":[{"id":"1","unknown_component":{}}]})", record, error));
	EXPECT_NE(error.find("unknown_component"), std::string::npos);
}

TEST(SceneJsonTest, rejects_unsupported_version)
{
	auto record = Sample();
	record.version = 2;
	std::string error;
	EXPECT_FALSE(SceneJson::Validate(record, error));
}

TEST(SceneJsonTest, rejects_missing_version)
{
	SceneRecord record;
	std::string error;
	EXPECT_FALSE(SceneJson::Parse(R"({"entities":[]})", record, error));
}

TEST(SceneJsonTest, rejects_duplicate_entity_ids)
{
	auto record = Sample();
	record.entities.push_back(record.entities.front());
	std::string error;
	EXPECT_FALSE(SceneJson::Validate(record, error));
}

TEST(SceneJsonTest, rejects_invalid_uuid_strings)
{
	for (const auto* id : { "0", "01", "-1", "1x", "18446744073709551616" })
	{
		auto record = Sample();
		record.entities[0].id = id;
		std::string error;
		EXPECT_FALSE(SceneJson::Validate(record, error)) << id;
	}
}

TEST(SceneJsonTest, rejects_numeric_uuid_to_prevent_precision_loss)
{
	SceneRecord record;
	std::string error;
	EXPECT_FALSE(SceneJson::Parse(R"({"version":1,"entities":[{"id":9007199254740993}]})", record, error));
}

TEST(SceneJsonTest, rejects_invalid_vector_and_integer_types)
{
	SceneRecord record;
	std::string error;
	EXPECT_FALSE(SceneJson::Parse(R"({"version":1,"entities":[{"id":"1","transform":{"translation":[1,2]}}]})", record, error));
	EXPECT_FALSE(SceneJson::Parse(R"({"version":1.5,"entities":[]})", record, error));
}

TEST(SceneJsonTest, rejects_nonfinite_numbers_before_writing)
{
	auto record = Sample();
	record.entities[0].transform.translation.x = std::numeric_limits<float>::infinity();
	std::string text, error;
	EXPECT_FALSE(SceneJson::Serialize(record, text, error));
}

TEST(SceneJsonTest, rejects_invalid_utf8_before_writing)
{
	auto record = Sample();
	record.name = std::string(1, static_cast<char>(0xff));
	std::string text, error;
	EXPECT_FALSE(SceneJson::Serialize(record, text, error));
}

TEST(SceneJsonTest, rejects_nonportable_asset_paths)
{
	for (const auto* path : { "../model.obj", "/model.obj", "C:/model.obj", "models\\model.obj", "models/../model.obj" })
	{
		auto record = Sample();
		record.entities[0].model->path = path;
		std::string error;
		EXPECT_FALSE(SceneJson::Validate(record, error)) << path;
	}
}

TEST(SceneJsonTest, rejects_primary_camera_without_camera_component)
{
	auto record = Sample();
	record.entities[0].camera.reset();
	std::string error;
	EXPECT_FALSE(SceneJson::Validate(record, error));
}

TEST(SceneJsonTest, rejects_camera_without_primary_selection)
{
	auto record = Sample();
	record.primary_camera.clear();
	std::string error;
	EXPECT_FALSE(SceneJson::Validate(record, error));
}

TEST(SceneJsonTest, rejects_invalid_projection_and_parallel_up)
{
	auto record = Sample();
	std::string error;
	record.entities[0].camera->near_plane = 500.f;
	EXPECT_FALSE(SceneJson::Validate(record, error));
	record.entities[0].camera->near_plane = 0.1f;
	record.entities[0].camera->up = { 1.f, 0.f, 0.f };
	EXPECT_FALSE(SceneJson::Validate(record, error));
}

TEST(SceneJsonTest, rejects_conflicting_body_components)
{
	auto record = Sample();
	record.entities[0].kinematic_body.emplace();
	std::string error;
	EXPECT_FALSE(SceneJson::Validate(record, error));
}

TEST(SceneJsonTest, model_derived_body_requires_model)
{
	auto record = Sample();
	record.entities[0].model.reset();
	record.entities[0].rigid_body->shape = "model_sphere";
	std::string error;
	EXPECT_FALSE(SceneJson::Validate(record, error));
}

TEST(SceneJsonTest, rejects_duplicate_scripts)
{
	auto record = Sample();
	record.entities[0].scripts.push_back(record.entities[0].scripts.front());
	std::string error;
	EXPECT_FALSE(SceneJson::Validate(record, error));
}

TEST(SceneJsonTest, sample_scenes_are_valid_without_graphics_or_external_assets)
{
	for (const auto* name : { "nuke.scene.json", "anor_londo.scene.json" })
	{
		SceneRecord record;
		std::string error, first, second;
		ASSERT_TRUE(SceneJson::Load(std::filesystem::path(DL_SCENE_FIXTURES) / name, record, error)) << error;
		ASSERT_TRUE(SceneJson::Serialize(record, first, error)) << error;
		SceneRecord reloaded;
		ASSERT_TRUE(SceneJson::Parse(first, reloaded, error)) << error;
		ASSERT_TRUE(SceneJson::Serialize(reloaded, second, error)) << error;
		EXPECT_EQ(first, second);
		EXPECT_FALSE(record.entities.empty());
	}
}
