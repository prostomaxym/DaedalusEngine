#include <ECS/Entity.h>
#include <ECS/Serialization/SceneJson.h>
#include <ECS/Serialization/SceneSerializer.h>
#include <ECS/Serialization/ScriptRegistry.h>

#include <gtest/gtest.h>
#include <chrono>
#include <limits>

namespace Daedalus
{
namespace
{
	const DeltaTime Step(std::chrono::milliseconds(10));

	void ExpectVector(const glm::vec3& actual, const glm::vec3& expected)
	{
		EXPECT_NEAR(actual.x, expected.x, 0.0001f);
		EXPECT_NEAR(actual.y, expected.y, 0.0001f);
		EXPECT_NEAR(actual.z, expected.z, 0.0001f);
	}

	void ExpectBodyPose(Scene& scene, Entity entity, RigidBodyHandle handle)
	{
		glm::vec3 position;
		glm::quat orientation;
		ASSERT_TRUE(scene.GetPhysicsWorld().GetRigidBodyTransform(handle, position, orientation));
		ExpectVector(glm::vec3((*scene.GetWorldTransform(entity.GetUUID()))[3]), position);
		EXPECT_NEAR(std::abs(glm::dot(scene.GetWorldOrientation(entity.GetUUID()), orientation)), 1.f, 0.0001f);
	}

	SceneRecord LegacyPoses()
	{
		SceneRecord record;
		record.version = 1;
		record.primary_camera = "1";
		EntityRecord entity;
		entity.id = "1";
		entity.name = "Player";
		entity.transform.translation = { 20.f, 0.f, 0.f };
		entity.camera.emplace();
		entity.camera->position = { 1.f, 2.f, 3.f };
		entity.camera->yaw = -90.f;
		entity.spot_light.emplace();
		entity.spot_light->position = { 4.f, 5.f, 6.f };
		entity.spot_light->direction = { 1.f, 0.f, 0.f };
		record.entities.push_back(entity);
		return record;
	}
}

TEST(HierarchyIntegrationTest, camera_and_all_light_types_derive_world_pose_from_trs)
{
	Scene scene;
	auto parent = scene.CreateEntity();
	auto child = scene.CreateEntity();
	parent.GetComponent<TransformComponent>().translation = { 10.f, 20.f, 30.f };
	parent.GetComponent<TransformComponent>().rotation = { 10.f, 20.f, 30.f };
	parent.GetComponent<TransformComponent>().scale = { 2.f, 3.f, 4.f };
	auto& local = child.GetComponent<TransformComponent>();
	local.translation = { 1.f, 2.f, 3.f };
	local.rotation = { 15.f, 25.f, 35.f };
	auto& camera = *child.AddComponent<CameraComponent>(CameraProjectionProps{}).camera;
	child.AddComponent<DirectionalLightComponent>();
	child.AddComponent<PointLightComponent>();
	child.AddComponent<SpotLightComponent>();
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	scene.UpdateWorldTransforms();
	const auto position = glm::vec3((*scene.GetWorldTransform(child.GetUUID()))[3]);
	const auto orientation = parent.GetComponent<TransformComponent>().GetOrientationQuat() * local.GetOrientationQuat();
	const auto direction = orientation * glm::vec3(0.f, 0.f, -1.f);
	ExpectVector(camera.GetPosition(), position);
	ExpectVector(camera.GetDirection(), direction);
	ExpectVector(camera.GetUp(), orientation * glm::vec3(0.f, 1.f, 0.f));
	ExpectVector(child.GetComponent<DirectionalLightComponent>().light.GetProps().direction, direction);
	ExpectVector(child.GetComponent<PointLightComponent>().light.GetProps().light_pos, position);
	ExpectVector(child.GetComponent<SpotLightComponent>().light.GetShaderSSBO().direction, direction);
	camera.SetPosition({ 100.f, 100.f, 100.f });
	child.GetComponent<SpotLightComponent>().light.SetDirection({ 1.f, 0.f, 0.f });
	EXPECT_EQ(scene.UpdateWorldTransforms(), 0u);
	ExpectVector(camera.GetPosition(), position);
	ExpectVector(child.GetComponent<SpotLightComponent>().light.GetProps().direction, direction);
}

TEST(HierarchyIntegrationTest, world_pose_writeback_preserves_local_scale_and_rejects_invalid_input)
{
	Scene scene;
	auto parent = scene.CreateEntity();
	auto child = scene.CreateEntity();
	parent.GetComponent<TransformComponent>().translation = { 10.f, 0.f, 0.f };
	parent.GetComponent<TransformComponent>().rotation.y = 90.f;
	child.GetComponent<TransformComponent>().scale = { 2.f, 3.f, 4.f };
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	scene.UpdateWorldTransforms();
	const auto rotation = glm::quat(glm::radians(glm::vec3(10.f, 20.f, 30.f)));
	ASSERT_TRUE(scene.SetWorldPose(child.GetUUID(), { 3.f, 4.f, 5.f }, rotation));
	scene.UpdateWorldTransforms();
	ExpectVector(glm::vec3((*scene.GetWorldTransform(child.GetUUID()))[3]), { 3.f, 4.f, 5.f });
	EXPECT_NEAR(std::abs(glm::dot(scene.GetWorldOrientation(child.GetUUID()), rotation)), 1.f, 0.0001f);
	ExpectVector(child.GetComponent<TransformComponent>().scale, { 2.f, 3.f, 4.f });
	EXPECT_FALSE(scene.SetWorldPose(child.GetUUID(), { std::numeric_limits<float>::infinity(), 0.f, 0.f }, rotation));
	EXPECT_FALSE(scene.SetWorldPose(UUID(0), {}, rotation));
}

TEST(HierarchyIntegrationTest, parented_kinematic_and_static_bodies_follow_ancestor_pose)
{
	Scene scene;
	auto parent = scene.CreateEntity();
	auto kinematic = scene.CreateEntity();
	auto rigid = scene.CreateEntity();
	kinematic.GetComponent<TransformComponent>().translation.x = 2.f;
	rigid.GetComponent<TransformComponent>().translation.x = -2.f;
	auto& kb = kinematic.AddComponent<KinematicBodyComponent>();
	auto& rb = rigid.AddComponent<RigidBodyComponent>();
	ASSERT_TRUE(scene.SetParent(kinematic.GetUUID(), parent.GetUUID()));
	ASSERT_TRUE(scene.SetParent(rigid.GetUUID(), parent.GetUUID()));
	for (const float yaw : { 0.f, 45.f, 90.f })
	{
		parent.GetComponent<TransformComponent>().translation.y = yaw;
		parent.GetComponent<TransformComponent>().rotation.y = yaw;
		scene.StepPhysics(Step);
		ExpectBodyPose(scene, kinematic, kb.handle);
		ExpectBodyPose(scene, rigid, rb.handle);
	}
}

TEST(HierarchyIntegrationTest, nested_dynamic_bodies_write_back_in_parent_before_child_order)
{
	Scene scene;
	scene.GetPhysicsWorld().SetGravity({ 0.f, -10.f, 0.f });
	auto parent = scene.CreateEntity();
	auto child = scene.CreateEntity();
	auto visual = scene.CreateEntity();
	BodyRecord body;
	body.mass = 1.f;
	body.position = { 0.f, 100.f, 0.f };
	parent.GetComponent<TransformComponent>().translation = body.position;
	auto& parent_body = parent.AddComponent<RigidBodyComponent>(body);
	child.GetComponent<TransformComponent>().translation.x = 10.f;
	auto& child_body = child.AddComponent<RigidBodyComponent>(body);
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	ASSERT_TRUE(scene.SetParent(visual.GetUUID(), child.GetUUID()));
	for (int tick = 0; tick < 10; ++tick)
	{
		scene.StepPhysics(Step);
		ExpectBodyPose(scene, parent, parent_body.handle);
		ExpectBodyPose(scene, child, child_body.handle);
		ExpectVector(glm::vec3((*scene.GetWorldTransform(visual.GetUUID()))[3]), glm::vec3((*scene.GetWorldTransform(child.GetUUID()))[3]));
	}
	EXPECT_LT(parent.GetComponent<TransformComponent>().translation.y, 100.f);
}

TEST(HierarchyIntegrationTest, moving_or_reparenting_ancestor_does_not_teleport_active_dynamic_body)
{
	Scene scene;
	scene.GetPhysicsWorld().SetGravity(glm::vec3(0.f));
	auto first = scene.CreateEntity();
	auto second = scene.CreateEntity();
	auto child = scene.CreateEntity();
	BodyRecord body;
	body.mass = 1.f;
	auto& rb = child.AddComponent<RigidBodyComponent>(body);
	child.GetComponent<TransformComponent>().translation.x = 2.f;
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), first.GetUUID()));
	scene.StepPhysics(Step);
	first.GetComponent<TransformComponent>().translation.x = 10.f;
	scene.StepPhysics(Step);
	ExpectBodyPose(scene, child, rb.handle);
	EXPECT_NEAR(child.GetComponent<TransformComponent>().translation.x, -8.f, 0.0001f);
	second.GetComponent<TransformComponent>().translation.x = 20.f;
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), second.GetUUID()));
	scene.StepPhysics(Step);
	ExpectBodyPose(scene, child, rb.handle);
	EXPECT_NEAR(child.GetComponent<TransformComponent>().translation.x, -18.f, 0.0001f);
}

TEST(HierarchyIntegrationTest, scaled_ancestor_reparenting_is_rejected_for_entire_body_subtree)
{
	Scene scene;
	auto scaled = scene.CreateEntity();
	auto visual = scene.CreateEntity();
	auto body = scene.CreateEntity();
	scaled.GetComponent<TransformComponent>().scale = glm::vec3(2.f);
	body.AddComponent<KinematicBodyComponent>();
	ASSERT_TRUE(scene.SetParent(body.GetUUID(), visual.GetUUID()));
	EXPECT_FALSE(scene.SetParent(visual.GetUUID(), scaled.GetUUID()));
	EXPECT_EQ(scene.GetParent(visual.GetUUID()), UUID(0));
	EXPECT_EQ(scene.GetParent(body.GetUUID()), visual.GetUUID());
}

TEST(HierarchyIntegrationTest, runtime_ancestor_scaling_detaches_body_at_last_physics_pose)
{
	Scene scene;
	auto parent = scene.CreateEntity();
	auto child = scene.CreateEntity();
	child.GetComponent<TransformComponent>().translation.x = 3.f;
	auto& body = child.AddComponent<KinematicBodyComponent>();
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	scene.StepPhysics(Step);
	parent.GetComponent<TransformComponent>().scale = { -1.f, 0.f, 2.f };
	scene.StepPhysics(Step);
	EXPECT_EQ(scene.GetParent(child.GetUUID()), UUID(0));
	EXPECT_NEAR(child.GetComponent<TransformComponent>().translation.x, 3.f, 0.0001f);
	ExpectBodyPose(scene, child, body.handle);
}

TEST(HierarchyIntegrationTest, legacy_conflicting_poses_become_linked_entities_without_moving_source)
{
	const auto legacy = LegacyPoses();
	SceneRecord upgraded;
	std::string error;
	ASSERT_TRUE(SceneJson::Upgrade(legacy, upgraded, error)) << error;
	ASSERT_EQ(upgraded.version, CurrentSceneVersion);
	ASSERT_EQ(upgraded.entities.size(), 3u);
	EXPECT_FALSE(upgraded.entities[0].camera);
	EXPECT_FALSE(upgraded.entities[0].spot_light);
	EXPECT_FLOAT_EQ(upgraded.entities[0].transform.translation.x, 20.f);
	Scene scene;
	ScriptRegistry scripts;
	ASSERT_TRUE(SceneSerializer::Instantiate(upgraded, scene, {}, scripts, error)) << error;
	auto source = scene.GetEntityByUUID(UUID(1));
	auto camera = source.GetLinkedEntity("camera");
	auto light = source.GetLinkedEntity("spot_light");
	ASSERT_TRUE(camera);
	ASSERT_TRUE(light);
	ASSERT_NE(scene.GetCamera(), nullptr);
	ExpectVector(scene.GetCamera()->GetPosition(), { 1.f, 2.f, 3.f });
	ExpectVector(scene.GetCamera()->GetDirection(), { 0.f, 0.f, -1.f });
	ExpectVector(light.GetComponent<SpotLightComponent>().light.GetProps().direction, { 1.f, 0.f, 0.f });
	ExpectVector(light.GetComponent<SpotLightComponent>().light.GetProps().light_pos, { 4.f, 5.f, 6.f });
	EXPECT_EQ(scene.GetCamera(), camera.GetComponent<CameraComponent>().camera.get());
	SceneRecord captured;
	ASSERT_TRUE(SceneSerializer::Capture(scene, {}, scripts, captured, error)) << error;
	EXPECT_EQ(captured.entities[0].links, upgraded.entities[0].links);
	std::string text;
	ASSERT_TRUE(SceneJson::Serialize(captured, text, error)) << error;
	EXPECT_EQ(text.find("\"yaw\""), std::string::npos);
	EXPECT_EQ(text.find("\"direction\""), std::string::npos);
	EXPECT_EQ(text.find("\"position\""), std::string::npos);
	SceneRecord parsed;
	ASSERT_TRUE(SceneJson::Parse(text, parsed, error)) << error;
	EXPECT_EQ(parsed.entities.size(), 3u);
}

TEST(HierarchyIntegrationTest, migration_is_deterministic_id_collision_safe_and_idempotent)
{
	auto legacy = LegacyPoses();
	EntityRecord occupied;
	occupied.id = "2";
	legacy.entities.push_back(occupied);
	occupied.id = "18446744073709551615";
	legacy.entities.push_back(occupied);
	SceneRecord first, second;
	std::string error, a, b;
	ASSERT_TRUE(SceneJson::Upgrade(legacy, first, error)) << error;
	EXPECT_EQ(first.entities[0].links.at("camera"), "3");
	ASSERT_TRUE(SceneJson::Upgrade(first, second, error)) << error;
	ASSERT_TRUE(SceneJson::Serialize(first, a, error));
	ASSERT_TRUE(SceneJson::Serialize(second, b, error));
	EXPECT_EQ(a, b);
}

TEST(HierarchyIntegrationTest, links_validate_round_trip_and_clear_when_target_is_destroyed)
{
	Scene scene;
	auto source = scene.CreateEntity();
	auto target = scene.CreateEntity();
	EXPECT_FALSE(scene.SetEntityLink(source.GetUUID(), "", target.GetUUID()));
	EXPECT_FALSE(scene.SetEntityLink(source.GetUUID(), "target", UUID(0)));
	ASSERT_TRUE(scene.SetEntityLink(source.GetUUID(), "target", target.GetUUID()));
	EXPECT_EQ(source.GetLinkedEntity("target"), target);
	scene.DestroyEntity(target);
	EXPECT_FALSE(source.GetLinkedEntity("target"));
	auto record = LegacyPoses();
	record.version = 2;
	record.entities[0].links["camera"] = "99";
	std::string error;
	EXPECT_FALSE(SceneJson::Validate(record, error));
}

TEST(HierarchyIntegrationTest, version_two_rejects_duplicate_camera_and_light_pose_fields)
{
	SceneRecord record;
	std::string error;
	EXPECT_FALSE(SceneJson::Parse(R"({"version":2,"primary_camera":"1","entities":[{"id":"1","camera":{"yaw":0}}]})", record, error));
	EXPECT_FALSE(SceneJson::Parse(R"({"version":2,"entities":[{"id":"1","spot_light":{"direction":[0,0,-1]}}]})", record, error));
}

TEST(HierarchyIntegrationTest, driven_descendants_follow_simulated_parent_in_same_tick)
{
	Scene scene;
	auto parent = scene.CreateEntity();
	auto kinematic = scene.CreateEntity();
	auto fixed = scene.CreateEntity();
	BodyRecord record;
	record.mass = 1.f;
	record.position.y = 10.f;
	parent.GetComponent<TransformComponent>().translation.y = 10.f;
	parent.AddComponent<RigidBodyComponent>(record);
	kinematic.GetComponent<TransformComponent>().translation.x = 5.f;
	fixed.GetComponent<TransformComponent>().translation.x = -5.f;
	auto& kb = kinematic.AddComponent<KinematicBodyComponent>();
	auto& rb = fixed.AddComponent<RigidBodyComponent>();
	ASSERT_TRUE(scene.SetParent(kinematic.GetUUID(), parent.GetUUID()));
	ASSERT_TRUE(scene.SetParent(fixed.GetUUID(), parent.GetUUID()));
	for (int tick = 0; tick < 10; ++tick)
	{
		scene.StepPhysics(Step);
		ExpectBodyPose(scene, kinematic, kb.handle);
		ExpectBodyPose(scene, fixed, rb.handle);
	}
}

TEST(HierarchyIntegrationTest, legacy_camera_split_retains_hierarchy_anchor_and_descendant_transform)
{
	auto legacy = LegacyPoses();
	legacy.entities[0].spot_light.reset();
	EntityRecord child;
	child.id = "2";
	child.parent = "1";
	child.transform.translation.x = 3.f;
	legacy.entities.push_back(child);
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	ASSERT_TRUE(SceneSerializer::Instantiate(legacy, scene, {}, scripts, error)) << error;
	EXPECT_EQ(scene.GetParent(UUID(2)), UUID(1));
	EXPECT_NEAR((*scene.GetWorldTransform(UUID(2)))[3].x, 23.f, 0.0001f);
	ExpectVector(scene.GetCamera()->GetPosition(), { 1.f, 2.f, 3.f });
}

TEST(HierarchyIntegrationTest, dedicated_legacy_camera_retains_id_and_custom_up)
{
	auto legacy = LegacyPoses();
	legacy.entities[0].spot_light.reset();
	legacy.entities[0].camera->up = { 1.f, 0.f, 0.f };
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	ASSERT_TRUE(SceneSerializer::Instantiate(legacy, scene, {}, scripts, error)) << error;
	auto entity = scene.GetEntityByUUID(UUID(1));
	ASSERT_TRUE(entity.HasComponent<CameraComponent>());
	EXPECT_FALSE(entity.GetLinkedEntity("camera"));
	ExpectVector(scene.GetCamera()->GetUp(), { 1.f, 0.f, 0.f });
	ExpectVector(scene.GetCamera()->GetPosition(), { 1.f, 2.f, 3.f });
}

TEST(HierarchyIntegrationTest, supported_parented_bodies_and_links_survive_capture_and_reload)
{
	Scene first, second;
	auto parent = first.CreateEntityWithUUID(UUID(1));
	auto child = first.CreateEntityWithUUID(UUID(2));
	parent.GetComponent<TransformComponent>().rotation.y = 90.f;
	child.GetComponent<TransformComponent>().translation.x = 4.f;
	child.AddComponent<KinematicBodyComponent>();
	ASSERT_TRUE(first.SetParent(child.GetUUID(), parent.GetUUID()));
	ASSERT_TRUE(first.SetEntityLink(parent.GetUUID(), "body", child.GetUUID()));
	SceneRecord record;
	ScriptRegistry scripts;
	std::string error;
	ASSERT_TRUE(SceneSerializer::Capture(first, {}, scripts, record, error)) << error;
	ASSERT_TRUE(SceneSerializer::Instantiate(record, second, {}, scripts, error)) << error;
	EXPECT_EQ(second.GetParent(UUID(2)), UUID(1));
	EXPECT_EQ(second.GetEntityLink(UUID(1), "body"), UUID(2));
	second.StepPhysics(Step);
	auto restored = second.GetEntityByUUID(UUID(2));
	ExpectBodyPose(second, restored, restored.GetComponent<KinematicBodyComponent>().handle);
}

TEST(HierarchyIntegrationTest, scaled_ancestor_body_addition_detaches_with_local_pose_retained)
{
	Scene scene;
	auto parent = scene.CreateEntity();
	auto child = scene.CreateEntity();
	parent.GetComponent<TransformComponent>().scale.x = 2.f;
	child.GetComponent<TransformComponent>().translation.x = 3.f;
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	child.AddComponent<KinematicBodyComponent>();
	EXPECT_EQ(scene.GetParent(child.GetUUID()), UUID(0));
	EXPECT_FLOAT_EQ(child.GetComponent<TransformComponent>().translation.x, 3.f);
}

TEST(HierarchyIntegrationTest, collision_callback_can_destroy_parent_before_transform_writeback)
{
	Scene scene;
	auto floor = scene.CreateEntity();
	auto parent = scene.CreateEntity();
	auto child = scene.CreateEntity();
	auto fixed = scene.CreateEntity();
	BodyRecord record;
	record.position.x = 10.f;
	floor.AddComponent<RigidBodyComponent>(record);
	record.mass = 1.f;
	parent.GetComponent<TransformComponent>().translation.x = 10.f;
	parent.AddComponent<RigidBodyComponent>(record);
	child.GetComponent<TransformComponent>().translation.x = 5.f;
	fixed.GetComponent<TransformComponent>().translation.x = -5.f;
	const auto kinematic_handle = child.AddComponent<KinematicBodyComponent>().handle;
	const auto rigid_handle = fixed.AddComponent<RigidBodyComponent>().handle;
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	ASSERT_TRUE(scene.SetParent(fixed.GetUUID(), parent.GetUUID()));
	const auto parent_id = parent.GetUUID();
	bool destroyed = false;
	scene.GetPhysicsWorld().RegisterCollisionCallback([&](RigidBodyHandle, RigidBodyHandle)
	{
		if (destroyed) return;
		destroyed = true;
		scene.DestroyEntity(parent);
	});
	scene.StepPhysics(Step);
	EXPECT_TRUE(destroyed);
	EXPECT_FALSE(scene.GetEntityByUUID(parent_id));
	EXPECT_EQ(scene.GetParent(child.GetUUID()), UUID(0));
	EXPECT_NEAR((*scene.GetWorldTransform(child.GetUUID()))[3].x, 5.f, 0.0001f);
	ExpectBodyPose(scene, child, kinematic_handle);
	ExpectBodyPose(scene, fixed, rigid_handle);
}

TEST(HierarchyIntegrationTest, root_static_collider_retains_independent_authored_creation_pose)
{
	Scene scene;
	auto entity = scene.CreateEntity();
	entity.GetComponent<TransformComponent>().translation = { 100.f, 200.f, 300.f };
	BodyRecord record;
	record.position = { 1.f, 2.f, 3.f };
	auto& body = entity.AddComponent<RigidBodyComponent>(record);
	scene.StepPhysics(Step);
	glm::vec3 position;
	glm::quat orientation;
	ASSERT_TRUE(scene.GetPhysicsWorld().GetRigidBodyTransform(body.handle, position, orientation));
	ExpectVector(position, record.position);
	ExpectVector(entity.GetComponent<TransformComponent>().translation, { 100.f, 200.f, 300.f });
}

TEST(HierarchyIntegrationTest, orphaned_static_body_follows_preserved_local_pose_once)
{
	Scene scene;
	auto parent = scene.CreateEntity();
	auto child = scene.CreateEntity();
	parent.GetComponent<TransformComponent>().translation.x = 10.f;
	child.GetComponent<TransformComponent>().translation.x = 3.f;
	auto& body = child.AddComponent<RigidBodyComponent>();
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	scene.StepPhysics(Step);
	scene.DestroyEntity(parent);
	scene.StepPhysics(Step);
	ExpectBodyPose(scene, child, body.handle);
	EXPECT_NEAR((*scene.GetWorldTransform(child.GetUUID()))[3].x, 3.f, 0.0001f);
}
}
