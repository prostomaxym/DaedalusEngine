#include <ECS/SceneManager.h>
#include <ECS/Scene.h>
#include <ECS/Entity.h>
#include <gtest/gtest.h>

using namespace Daedalus;

TEST(SceneManagerTest, requests_do_not_replace_scene_until_commit)
{
	SceneManager manager;
	auto first = std::make_unique<Scene>();
	auto* original = first.get();
	manager.RequestScene(std::move(first), false);
	EXPECT_EQ(manager.GetActiveScene(), nullptr);
	ASSERT_TRUE(manager.CommitPending());
	EXPECT_EQ(manager.GetActiveScene(), original);
	auto next = std::make_unique<Scene>();
	auto* replacement = next.get();
	manager.RequestScene(std::move(next), false);
	EXPECT_EQ(manager.GetActiveScene(), original);
	ASSERT_TRUE(manager.CommitPending());
	EXPECT_EQ(manager.GetActiveScene(), replacement);
}

TEST(SceneManagerTest, replacement_destroys_old_scene_without_affecting_pending_world)
{
	SceneManager manager;
	auto scene = std::make_unique<Scene>();
	auto entity = scene->CreateEntity();
	entity.AddComponent<RigidBodyComponent>(1.f, 0.5f, glm::vec3(0.f));
	auto detached = entity.PopComponent<RigidBodyComponent>();
	manager.RequestScene(std::move(scene), false);
	ASSERT_TRUE(manager.CommitPending());
	manager.RequestScene(std::make_unique<Scene>(), false);
	ASSERT_TRUE(manager.CommitPending());
	detached.reset();
	EXPECT_NE(manager.GetActiveScene(), nullptr);
}

TEST(SceneManagerTest, null_request_unloads_at_next_boundary)
{
	SceneManager manager;
	manager.RequestScene(std::make_unique<Scene>(), false);
	ASSERT_TRUE(manager.CommitPending());
	manager.RequestScene(nullptr);
	EXPECT_NE(manager.GetActiveScene(), nullptr);
	ASSERT_TRUE(manager.CommitPending());
	EXPECT_EQ(manager.GetActiveScene(), nullptr);
}

TEST(SceneManagerTest, clear_cancels_pending_and_active_scenes)
{
	SceneManager manager;
	manager.RequestScene(std::make_unique<Scene>(), false);
	manager.Clear();
	ASSERT_TRUE(manager.CommitPending());
	EXPECT_EQ(manager.GetActiveScene(), nullptr);
	manager.RequestScene(std::make_unique<Scene>(), false);
	ASSERT_TRUE(manager.CommitPending());
	manager.Clear();
	EXPECT_EQ(manager.GetActiveScene(), nullptr);
}
