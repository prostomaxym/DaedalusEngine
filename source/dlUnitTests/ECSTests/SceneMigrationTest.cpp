#include <ECS/Serialization/SceneJson.h>
#include <gtest/gtest.h>

namespace Daedalus
{
TEST(SceneMigrationTest, every_supported_version_advances_to_current_without_changing_metadata)
{
	for (uint32_t version = 1; version <= CurrentSceneVersion; ++version)
	{
		SceneRecord source, upgraded;
		source.version = version;
		source.name = "Preserved";
		std::string error;
		ASSERT_TRUE(SceneJson::Upgrade(source, upgraded, error)) << error;
		EXPECT_EQ(upgraded.version, CurrentSceneVersion);
		EXPECT_EQ(upgraded.name, "Preserved");
		EXPECT_EQ(source.version, version);
	}
}

TEST(SceneMigrationTest, version_two_additive_upgrade_preserves_entity_configuration)
{
	SceneRecord source, upgraded;
	source.version = 2;
	source.primary_camera = "1";
	EntityRecord entity;
	entity.id = "1";
	entity.camera.emplace();
	entity.transform.translation = { 1.f, 2.f, 3.f };
	entity.camera->movement_speed = 15.f;
	source.entities.push_back(entity);
	std::string error, actual, expected;
	ASSERT_TRUE(SceneJson::Upgrade(source, upgraded, error)) << error;
	source.version = CurrentSceneVersion;
	ASSERT_TRUE(SceneJson::Serialize(source, expected, error));
	ASSERT_TRUE(SceneJson::Serialize(upgraded, actual, error));
	EXPECT_EQ(actual, expected);
}

TEST(SceneMigrationTest, legacy_pose_conversion_runs_before_additive_version_transition)
{
	SceneRecord source, upgraded;
	source.version = 1;
	source.primary_camera = "1";
	EntityRecord entity;
	entity.id = "1";
	entity.camera.emplace();
	entity.camera->position.x = 5.f;
	entity.point_light.emplace();
	source.entities.push_back(entity);
	std::string error;
	ASSERT_TRUE(SceneJson::Upgrade(source, upgraded, error)) << error;
	EXPECT_EQ(upgraded.version, CurrentSceneVersion);
	ASSERT_EQ(upgraded.entities.size(), 3u);
	EXPECT_FALSE(upgraded.entities[0].camera);
	EXPECT_EQ(upgraded.entities[0].links.at("camera"), upgraded.primary_camera);
	EXPECT_FLOAT_EQ(upgraded.entities[1].transform.translation.x, 5.f);
}

TEST(SceneMigrationTest, unsupported_or_invalid_versions_leave_destination_unchanged)
{
	for (const uint32_t version : { 0u, CurrentSceneVersion + 1 })
	{
		SceneRecord source, output;
		source.version = version;
		output.name = "Unchanged";
		std::string error;
		EXPECT_FALSE(SceneJson::Upgrade(source, output, error));
		EXPECT_EQ(output.name, "Unchanged");
		EXPECT_FALSE(error.empty());
	}
}

TEST(SceneMigrationTest, in_place_upgrade_and_repeated_upgrade_are_idempotent)
{
	SceneRecord record;
	record.version = 1;
	std::string error, first, second;
	ASSERT_TRUE(SceneJson::Upgrade(record, record, error)) << error;
	ASSERT_TRUE(SceneJson::Serialize(record, first, error));
	ASSERT_TRUE(SceneJson::Upgrade(record, record, error)) << error;
	ASSERT_TRUE(SceneJson::Serialize(record, second, error));
	EXPECT_EQ(first, second);
}

TEST(SceneMigrationTest, version_three_upgrade_preserves_prefab_references_and_exports)
{
	SceneRecord source, upgraded;
	source.version = 3;
	source.name = "Prefab scene";
	source.primary_camera = "10";
	EntityRecord anchor;
	anchor.id = "10";
	anchor.prefab = PrefabRecord{ "unit.json", { { "body", "1" } } };
	source.entities.push_back(anchor);
	std::string error, expected, actual;
	ASSERT_TRUE(SceneJson::Upgrade(source, upgraded, error)) << error;
	EXPECT_EQ(source.version, 3u);
	source.version = CurrentSceneVersion;
	ASSERT_TRUE(SceneJson::Serialize(source, expected, error));
	ASSERT_TRUE(SceneJson::Serialize(upgraded, actual, error));
	EXPECT_EQ(expected, actual);
}

TEST(SceneMigrationTest, legacy_exports_do_not_emit_version_four_fields)
{
	for (uint32_t version = 1; version <= 3; ++version)
	{
		SceneRecord record;
		record.version = version;
		if (version == 3)
		{
			EntityRecord anchor;
			anchor.id = "1";
			anchor.prefab = PrefabRecord{ "unit.json", {} };
			record.entities.push_back(anchor);
		}
		std::string text, error;
		ASSERT_TRUE(SceneJson::Serialize(record, text, error)) << error;
		EXPECT_EQ(text.find("overrides"), std::string::npos);
		EXPECT_EQ(text.find("base_prefab"), std::string::npos);
	}
}

TEST(SceneMigrationTest, legacy_versions_cannot_smuggle_new_prefab_fields)
{
	for (const uint32_t version : { 1u, 2u })
	{
		SceneRecord source;
		source.version = version;
		EntityRecord entity;
		entity.id = "1";
		entity.prefab = PrefabRecord{ "unit.json", {} };
		source.entities.push_back(entity);
		std::string error;
		EXPECT_FALSE(SceneJson::Validate(source, error));
		EXPECT_NE(error.find("version 3"), std::string::npos);
	}
}
}
