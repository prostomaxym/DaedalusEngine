#include <ECS/Entity.h>
#include <ECS/Serialization/SceneJson.h>
#include <ECS/Serialization/SceneSerializer.h>
#include <ECS/Serialization/ScriptRegistry.h>

#include <gtest/gtest.h>

namespace Daedalus
{

namespace
{
	void ExpectMatrix(const glm::mat4* actual, const glm::mat4& expected)
	{
		ASSERT_NE(actual, nullptr);
		for (int column = 0; column < 4; ++column)
			for (int row = 0; row < 4; ++row)
				EXPECT_NEAR((*actual)[column][row], expected[column][row], 0.0001f);
	}

	SceneRecord HierarchyRecord()
	{
		SceneRecord record;
		EntityRecord child;
		child.id = "1";
		child.parent = "2";
		child.transform.translation = { 2.f, 0.f, 0.f };
		EntityRecord parent;
		parent.id = "2";
		parent.transform.translation = { 10.f, 0.f, 0.f };
		record.entities = { child, parent };
		return record;
	}
}

TEST(TransformHierarchyTest, flat_scene_world_transform_matches_local_trs)
{
	Scene scene;
	auto entity = scene.CreateEntity();
	auto& local = entity.GetComponent<TransformComponent>();
	local.translation = { 1.f, 2.f, 3.f };
	local.rotation = { 12.f, 34.f, 56.f };
	local.scale = { 2.f, 3.f, 4.f };
	EXPECT_EQ(scene.UpdateWorldTransforms(), 1u);
	ExpectMatrix(scene.GetWorldTransform(entity.GetUUID()), local.GetTransform());
}

TEST(TransformHierarchyTest, composes_parent_rotation_scale_and_child_translation)
{
	Scene scene;
	auto parent = scene.CreateEntity();
	auto child = scene.CreateEntity();
	auto grandchild = scene.CreateEntity();
	parent.GetComponent<TransformComponent>().rotation.z = 90.f;
	parent.GetComponent<TransformComponent>().scale = { 2.f, 3.f, 4.f };
	child.GetComponent<TransformComponent>().translation.x = 2.f;
	grandchild.GetComponent<TransformComponent>().translation.y = 3.f;
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	ASSERT_TRUE(scene.SetParent(grandchild.GetUUID(), child.GetUUID()));
	EXPECT_EQ(scene.UpdateWorldTransforms(), 3u);
	const auto expected = parent.GetComponent<TransformComponent>().GetTransform()
		* child.GetComponent<TransformComponent>().GetTransform()
		* grandchild.GetComponent<TransformComponent>().GetTransform();
	ExpectMatrix(scene.GetWorldTransform(grandchild.GetUUID()), expected);
}

TEST(TransformHierarchyTest, unchanged_hierarchy_reuses_cached_world_matrices)
{
	Scene scene;
	auto parent = scene.CreateEntity();
	auto child = scene.CreateEntity();
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	EXPECT_EQ(scene.UpdateWorldTransforms(), 2u);
	EXPECT_EQ(scene.UpdateWorldTransforms(), 0u);
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	EXPECT_EQ(scene.UpdateWorldTransforms(), 0u);
}

TEST(TransformHierarchyTest, ancestor_edit_invalidates_descendants_but_not_other_roots)
{
	Scene scene;
	auto parent = scene.CreateEntity();
	auto child = scene.CreateEntity();
	auto other = scene.CreateEntity();
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	scene.UpdateWorldTransforms();
	parent.GetComponent<TransformComponent>().translation.x = 7.f;
	EXPECT_EQ(scene.UpdateWorldTransforms(), 2u);
	EXPECT_FLOAT_EQ((*scene.GetWorldTransform(child.GetUUID()))[3].x, 7.f);
	EXPECT_FLOAT_EQ((*scene.GetWorldTransform(other.GetUUID()))[3].x, 0.f);
	EXPECT_EQ(scene.UpdateWorldTransforms(), 0u);
}

TEST(TransformHierarchyTest, child_edit_does_not_invalidate_parent_or_sibling)
{
	Scene scene;
	auto parent = scene.CreateEntity();
	auto child = scene.CreateEntity();
	auto sibling = scene.CreateEntity();
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	ASSERT_TRUE(scene.SetParent(sibling.GetUUID(), parent.GetUUID()));
	scene.UpdateWorldTransforms();
	child.GetComponent<TransformComponent>().rotation.y = 45.f;
	child.GetComponent<TransformComponent>().scale.x = 3.f;
	EXPECT_EQ(scene.UpdateWorldTransforms(), 1u);
	ExpectMatrix(scene.GetWorldTransform(child.GetUUID()), child.GetComponent<TransformComponent>().GetTransform());
}

TEST(TransformHierarchyTest, reparenting_preserves_local_trs_and_updates_links)
{
	Scene scene;
	auto first = scene.CreateEntity();
	auto second = scene.CreateEntity();
	auto child = scene.CreateEntity();
	first.GetComponent<TransformComponent>().translation.x = 10.f;
	second.GetComponent<TransformComponent>().translation.x = 20.f;
	child.GetComponent<TransformComponent>().translation.x = 2.f;
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), first.GetUUID()));
	scene.UpdateWorldTransforms();
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), second.GetUUID()));
	EXPECT_EQ(scene.UpdateWorldTransforms(), 1u);
	EXPECT_TRUE(scene.GetChildren(first.GetUUID()).empty());
	ASSERT_EQ(scene.GetChildren(second.GetUUID()).size(), 1u);
	EXPECT_EQ(scene.GetChildren(second.GetUUID())[0], child.GetUUID());
	EXPECT_EQ(scene.GetParent(child.GetUUID()), second.GetUUID());
	EXPECT_FLOAT_EQ(child.GetComponent<TransformComponent>().translation.x, 2.f);
	EXPECT_FLOAT_EQ((*scene.GetWorldTransform(child.GetUUID()))[3].x, 22.f);
	ASSERT_TRUE(scene.SetParent(child.GetUUID()));
	EXPECT_EQ(scene.UpdateWorldTransforms(), 1u);
	EXPECT_FLOAT_EQ((*scene.GetWorldTransform(child.GetUUID()))[3].x, 2.f);
}

TEST(TransformHierarchyTest, rejects_self_parenting_cycles_and_unknown_entities)
{
	Scene scene;
	auto parent = scene.CreateEntityWithUUID(UUID(1));
	auto child = scene.CreateEntityWithUUID(UUID(2));
	auto grandchild = scene.CreateEntityWithUUID(UUID(3));
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	ASSERT_TRUE(scene.SetParent(grandchild.GetUUID(), child.GetUUID()));
	EXPECT_FALSE(scene.SetParent(parent.GetUUID(), grandchild.GetUUID()));
	EXPECT_FALSE(scene.SetParent(child.GetUUID(), child.GetUUID()));
	EXPECT_FALSE(scene.SetParent(UUID(0), parent.GetUUID()));
	EXPECT_FALSE(scene.SetParent(child.GetUUID(), UUID(0x1234)));
	EXPECT_EQ(scene.GetParent(child.GetUUID()), parent.GetUUID());
	EXPECT_EQ(scene.GetParent(parent.GetUUID()), UUID(0));
	EXPECT_EQ(scene.GetWorldTransform(UUID(0)), nullptr);
	EXPECT_TRUE(scene.GetChildren(UUID(0)).empty());
}

TEST(TransformHierarchyTest, destroying_parent_orphans_children_without_changing_local_trs)
{
	Scene scene;
	auto parent = scene.CreateEntity();
	auto child = scene.CreateEntity();
	auto grandchild = scene.CreateEntity();
	parent.GetComponent<TransformComponent>().translation.x = 10.f;
	child.GetComponent<TransformComponent>().translation.x = 2.f;
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	ASSERT_TRUE(scene.SetParent(grandchild.GetUUID(), child.GetUUID()));
	scene.UpdateWorldTransforms();
	const auto parent_id = parent.GetUUID();
	scene.DestroyEntity(parent);
	EXPECT_EQ(scene.GetParent(child.GetUUID()), UUID(0));
	EXPECT_EQ(scene.GetParent(grandchild.GetUUID()), child.GetUUID());
	EXPECT_EQ(scene.GetWorldTransform(parent_id), nullptr);
	EXPECT_EQ(scene.UpdateWorldTransforms(), 2u);
	EXPECT_FLOAT_EQ((*scene.GetWorldTransform(grandchild.GetUUID()))[3].x, 2.f);
}

TEST(TransformHierarchyTest, destroying_child_removes_parent_link)
{
	Scene scene;
	auto parent = scene.CreateEntity();
	auto child = scene.CreateEntity();
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	scene.DestroyEntity(child);
	EXPECT_TRUE(scene.GetChildren(parent.GetUUID()).empty());
	EXPECT_EQ(scene.UpdateWorldTransforms(), 1u);
}

TEST(TransformHierarchyTest, foreign_entity_cannot_destroy_local_hierarchy)
{
	Scene first, second;
	auto local = first.CreateEntity();
	auto foreign = second.CreateEntity();
	first.DestroyEntity(foreign);
	EXPECT_TRUE(first.GetEntityByUUID(local.GetUUID()));
	EXPECT_TRUE(second.GetEntityByUUID(foreign.GetUUID()));
}

TEST(TransformHierarchyTest, clear_discards_hierarchy_before_reusing_scene)
{
	Scene scene;
	auto parent = scene.CreateEntityWithUUID(UUID(1));
	auto child = scene.CreateEntityWithUUID(UUID(2));
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	scene.UpdateWorldTransforms();
	scene.Clear();
	EXPECT_EQ(scene.UpdateWorldTransforms(), 0u);
	EXPECT_EQ(scene.GetWorldTransform(UUID(2)), nullptr);
	auto replacement = scene.CreateEntityWithUUID(UUID(2));
	EXPECT_EQ(scene.GetParent(replacement.GetUUID()), UUID(0));
	EXPECT_EQ(scene.UpdateWorldTransforms(), 1u);
	ExpectMatrix(scene.GetWorldTransform(replacement.GetUUID()), glm::mat4(1.f));
}

TEST(TransformHierarchyTest, deep_hierarchy_updates_iteratively)
{
	Scene scene;
	constexpr uint64_t depth = 1024;
	for (uint64_t id = 1; id <= depth; ++id)
	{
		auto entity = scene.CreateEntityWithUUID(UUID(id));
		entity.GetComponent<TransformComponent>().translation.x = 1.f;
		if (id > 1) ASSERT_TRUE(scene.SetParent(UUID(id), UUID(id - 1)));
	}
	EXPECT_EQ(scene.UpdateWorldTransforms(), depth);
	EXPECT_FLOAT_EQ((*scene.GetWorldTransform(UUID(depth)))[3].x, static_cast<float>(depth));
	EXPECT_EQ(scene.UpdateWorldTransforms(), 0u);
}

TEST(TransformHierarchyTest, transform_replacement_and_removal_invalidate_cached_world)
{
	Scene scene;
	auto entity = scene.CreateEntity();
	scene.UpdateWorldTransforms();
	entity.AddOrReplaceComponent<TransformComponent>(glm::vec3(3.f, 4.f, 5.f));
	EXPECT_EQ(scene.UpdateWorldTransforms(), 1u);
	ExpectMatrix(scene.GetWorldTransform(entity.GetUUID()), entity.GetComponent<TransformComponent>().GetTransform());
	entity.RemoveComponent<TransformComponent>();
	EXPECT_EQ(scene.UpdateWorldTransforms(), 1u);
	ExpectMatrix(scene.GetWorldTransform(entity.GetUUID()), glm::mat4(1.f));
	EXPECT_EQ(scene.UpdateWorldTransforms(), 0u);
}

TEST(TransformHierarchyTest, physics_bodies_accept_unit_scale_parents)
{
	Scene scene;
	auto parent = scene.CreateEntity();
	auto rigid = scene.CreateEntity();
	auto kinematic = scene.CreateEntity();
	rigid.AddComponent<RigidBodyComponent>();
	kinematic.AddComponent<KinematicBodyComponent>();
	EXPECT_TRUE(scene.SetParent(rigid.GetUUID(), parent.GetUUID()));
	EXPECT_TRUE(scene.SetParent(kinematic.GetUUID(), parent.GetUUID()));
	EXPECT_FALSE(scene.SetParent(parent.GetUUID(), rigid.GetUUID()));
}

TEST(TransformHierarchyTest, adding_physics_body_preserves_valid_parent)
{
	Scene scene;
	auto parent = scene.CreateEntity();
	auto rigid = scene.CreateEntity();
	auto kinematic = scene.CreateEntity();
	ASSERT_TRUE(scene.SetParent(rigid.GetUUID(), parent.GetUUID()));
	ASSERT_TRUE(scene.SetParent(kinematic.GetUUID(), parent.GetUUID()));
	rigid.GetComponent<TransformComponent>().translation.x = 3.f;
	rigid.AddComponent<RigidBodyComponent>();
	kinematic.AddComponent<KinematicBodyComponent>();
	EXPECT_EQ(scene.GetParent(rigid.GetUUID()), parent.GetUUID());
	EXPECT_EQ(scene.GetParent(kinematic.GetUUID()), parent.GetUUID());
	EXPECT_EQ(scene.GetChildren(parent.GetUUID()).size(), 2u);
	EXPECT_FLOAT_EQ(rigid.GetComponent<TransformComponent>().translation.x, 3.f);
}

TEST(TransformHierarchyTest, persistence_restores_forward_parent_references_and_local_trs)
{
	Scene first, second;
	ScriptRegistry scripts;
	SceneRecord captured, parsed;
	std::string error, text;
	ASSERT_TRUE(SceneSerializer::Instantiate(HierarchyRecord(), first, {}, scripts, error)) << error;
	EXPECT_EQ(first.GetParent(UUID(1)), UUID(2));
	EXPECT_FLOAT_EQ((*first.GetWorldTransform(UUID(1)))[3].x, 12.f);
	ASSERT_TRUE(SceneSerializer::Capture(first, {}, scripts, captured, error)) << error;
	ASSERT_EQ(captured.entities[0].parent, "2");
	EXPECT_FLOAT_EQ(captured.entities[0].transform.translation.x, 2.f);
	ASSERT_TRUE(SceneJson::Serialize(captured, text, error)) << error;
	ASSERT_TRUE(SceneJson::Parse(text, parsed, error)) << error;
	ASSERT_TRUE(SceneSerializer::Instantiate(parsed, second, {}, scripts, error)) << error;
	EXPECT_EQ(second.GetParent(UUID(1)), UUID(2));
	EXPECT_FLOAT_EQ((*second.GetWorldTransform(UUID(1)))[3].x, 12.f);
}

TEST(TransformHierarchyTest, persistence_rejects_cycles_before_mutating_scene)
{
	auto record = HierarchyRecord();
	record.entities[1].parent = "1";
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	EXPECT_FALSE(SceneSerializer::Instantiate(record, scene, {}, scripts, error));
	EXPECT_FALSE(error.empty());
	EXPECT_TRUE(scene.IsEmpty());
}

TEST(TransformHierarchyTest, persistence_rejects_missing_parent_and_scaled_physics_ancestors)
{
	std::string error;
	auto record = HierarchyRecord();
	record.entities[0].parent = "3";
	EXPECT_FALSE(SceneJson::Validate(record, error));
	record = HierarchyRecord();
	record.entities[1].transform.scale.x = 2.f;
	record.entities[0].rigid_body.emplace();
	EXPECT_FALSE(SceneJson::Validate(record, error));
	record.entities[0].rigid_body.reset();
	record.entities[0].kinematic_body.emplace();
	EXPECT_FALSE(SceneJson::Validate(record, error));
}
}
