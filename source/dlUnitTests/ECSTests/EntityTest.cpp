#include <ECS/Entity.h>
#include <ECS/Scene.h>

#include <gtest/gtest.h>

using namespace Daedalus;

TEST(EntityTest, default_entity_is_invalid)
{
	const Entity entity;

	EXPECT_FALSE(entity.IsValid());
	EXPECT_FALSE(static_cast<bool>(entity));
}

TEST(EntityTest, default_entities_are_equal)
{
	EXPECT_EQ(Entity(), Entity());
}

TEST(EntityTest, add_has_and_remove_component)
{
	Scene scene;
	Entity entity = scene.CreateEntity();
	ASSERT_FALSE(entity.HasComponent<RigidBodyComponent>());

	entity.AddComponent<RigidBodyComponent>();
	EXPECT_TRUE(entity.HasComponent<RigidBodyComponent>());

	entity.RemoveComponent<RigidBodyComponent>();
	EXPECT_FALSE(entity.HasComponent<RigidBodyComponent>());
}

TEST(EntityTest, get_component_returns_modifiable_reference)
{
	Scene scene;
	Entity entity = scene.CreateEntity();

	entity.GetComponent<TransformComponent>().translation = glm::vec3(1.f, 2.f, 3.f);

	EXPECT_EQ(entity.GetComponent<TransformComponent>().translation, glm::vec3(1.f, 2.f, 3.f));
}

TEST(EntityTest, add_or_replace_component_overwrites_existing)
{
	Scene scene;
	Entity entity = scene.CreateEntity();
	entity.GetComponent<TransformComponent>().translation = glm::vec3(1.f);

	entity.AddOrReplaceComponent<TransformComponent>(glm::vec3(5.f, 6.f, 7.f));

	EXPECT_EQ(entity.GetComponent<TransformComponent>().translation, glm::vec3(5.f, 6.f, 7.f));
}

TEST(EntityTest, entities_in_same_scene_are_distinct)
{
	Scene scene;

	EXPECT_NE(scene.CreateEntity("A"), scene.CreateEntity("A"));
}
