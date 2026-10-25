#include <ECS/Entity.h>
#include <ECS/Scene.h>

#include <gtest/gtest.h>

using namespace Daedalus;

TEST(SceneTest, new_scene_is_not_running_or_paused)
{
	Scene scene;

	EXPECT_FALSE(scene.IsRunning());
	EXPECT_FALSE(scene.IsPaused());
	EXPECT_EQ(scene.GetCamera(), nullptr);
}

TEST(SceneTest, set_paused_updates_state)
{
	Scene scene;
	scene.SetPaused(true);

	EXPECT_TRUE(scene.IsPaused());
}

TEST(SceneTest, created_entity_has_default_components_and_name)
{
	Scene scene;
	Entity entity = scene.CreateEntity();

	EXPECT_TRUE(entity.IsValid());
	EXPECT_EQ(entity.GetName(), "Entity");
	EXPECT_TRUE(entity.HasComponent<IDComponent>());
	EXPECT_TRUE(entity.HasComponent<TagComponent>());
	EXPECT_TRUE(entity.HasComponent<TransformComponent>());
}

TEST(SceneTest, created_entity_uses_given_name)
{
	Scene scene;
	Entity entity = scene.CreateEntity("Player");

	EXPECT_EQ(entity.GetName(), "Player");
}

TEST(SceneTest, create_entity_with_uuid_keeps_uuid)
{
	Scene scene;
	Entity entity = scene.CreateEntityWithUUID(Daedalus::UUID(777u), "Enemy");

	EXPECT_EQ(static_cast<uint64_t>(entity.GetUUID()), 777u);
}

TEST(SceneTest, find_entity_by_name_returns_matching_entity)
{
	Scene scene;
	scene.CreateEntity("A");
	Entity b = scene.CreateEntity("B");

	const Entity found = scene.FindEntityByName("B");

	EXPECT_TRUE(found.IsValid());
	EXPECT_EQ(found, b);
}

TEST(SceneTest, find_entity_by_unknown_name_returns_invalid_entity)
{
	Scene scene;
	scene.CreateEntity("A");

	EXPECT_FALSE(scene.FindEntityByName("Missing").IsValid());
}

TEST(SceneTest, get_entity_by_uuid_returns_matching_entity)
{
	Scene scene;
	Entity entity = scene.CreateEntityWithUUID(Daedalus::UUID(5u), "A");

	EXPECT_EQ(scene.GetEntityByUUID(Daedalus::UUID(5u)), entity);
	EXPECT_FALSE(scene.GetEntityByUUID(Daedalus::UUID(6u)).IsValid());
}

TEST(SceneTest, destroyed_entity_cannot_be_found)
{
	Scene scene;
	Entity entity = scene.CreateEntityWithUUID(Daedalus::UUID(9u), "Doomed");

	scene.DestroyEntity(entity);

	EXPECT_FALSE(scene.FindEntityByName("Doomed").IsValid());
	EXPECT_FALSE(scene.GetEntityByUUID(Daedalus::UUID(9u)).IsValid());
}

TEST(SceneTest, entities_from_different_scenes_are_not_equal)
{
	Scene first;
	Scene second;

	EXPECT_NE(first.CreateEntity("A"), second.CreateEntity("A"));
}
