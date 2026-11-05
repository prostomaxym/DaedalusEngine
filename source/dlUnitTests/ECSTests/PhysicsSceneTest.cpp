#include <ECS/Entity.h>
#include <ECS/NativeScript.h>
#include <ECS/Serialization/SceneJson.h>
#include <ECS/Serialization/SceneComposer.h>
#include <ECS/Serialization/SceneSerializer.h>
#include <ECS/Serialization/ScriptRegistry.h>
#include <Events/PhysicsEvent.h>
#include <Events/EventsEngine.h>
#include <Utils/VirtualFileSystem.h>
#include <gtest/gtest.h>
#include <zip.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>

namespace Daedalus
{
namespace
{
	SceneRecord PhysicsScene()
	{
		SceneRecord record;
		record.name = "Physics";
		EntityRecord first;
		first.id = "1";
		first.rigid_body.emplace();
		first.rigid_body->mass = 1.f;
		JointRecord joint;
		joint.target = "2";
		joint.pivot_a.x = 3.f;
		first.joints.push_back(joint);
		EntityRecord second;
		second.id = "2";
		second.transform.translation.x = 3.f;
		second.rigid_body.emplace();
		second.rigid_body->position.x = 3.f;
		EntityRecord character;
		character.id = "3";
		character.transform.translation = { 10.f, 2.f, 0.f };
		character.character_controller.emplace();
		record.entities = { first, second, character };
		return record;
	}

	void Step(Scene& scene, int count = 1)
	{
		for (int i = 0; i < count; ++i) scene.StepPhysics(DeltaTime(std::chrono::nanoseconds(16666667)));
	}

	class PhysicsProbeScript : public NativeScript
	{
	public:
		using NativeScript::NativeScript;
	};
}

TEST(PhysicsSchemaTest, round_trips_every_shape_and_setting)
{
	for (const auto* shape : { "box", "sphere", "capsule", "cylinder", "cone", "convex_hull", "triangle_mesh" })
	{
		SCOPED_TRACE(shape);
		auto source = PhysicsScene();
		auto& body = *source.entities[1].rigid_body;
		body.shape = shape;
		body.height = 2.f;
		body.radius = 0.75f;
		if (body.shape == "convex_hull") body.vertices = { { -1.f, -1.f, -1.f }, { 1.f, -1.f, -1.f }, { 0.f, 1.f, -1.f }, { 0.f, 0.f, 1.f } };
		if (body.shape == "triangle_mesh")
		{
			body.vertices = { { -1.f, 0.f, -1.f }, { 1.f, 0.f, -1.f }, { 0.f, 0.f, 1.f } };
			body.indices = { 0, 2, 1 };
		}
		body.settings = { 4, 17, true, 0.9f, 0.4f, 0.2f, 0.1f, 0.3f };
		auto& character = *source.entities[2].character_controller;
		character = { 0.6f, 1.4f, 0.4f, 30.f, 6.f, 40.f, { 0.f, -8.f, 0.f }, 8, 15 };
		source.entities[0].joints.clear();
		for (const auto* type : { "fixed", "point", "hinge", "slider", "cone" })
		{
			JointRecord joint;
			joint.target = "2";
			joint.type = type;
			joint.rotation_a = { 10.f, 20.f, 30.f };
			joint.rotation_b = { 20.f, 30.f, 40.f };
			joint.lower_limit = -10.f;
			joint.upper_limit = 20.f;
			joint.angular_lower_limit = -30.f;
			joint.angular_upper_limit = 40.f;
			joint.swing_limits = { 30.f, 40.f, 50.f };
			joint.collide_connected = true;
			joint.breaking_impulse = 10.f;
			source.entities[0].joints.push_back(joint);
		}
		std::string text, again, error;
		SceneRecord parsed;
		ASSERT_TRUE(SceneJson::Serialize(source, text, error)) << error;
		ASSERT_TRUE(SceneJson::Parse(text, parsed, error)) << error;
		ASSERT_TRUE(SceneJson::Serialize(parsed, again, error)) << error;
		EXPECT_EQ(text, again);
		EXPECT_EQ(parsed.version, 5u);
		EXPECT_EQ(parsed.entities[0].joints.size(), 5u);
	}
}

TEST(PhysicsSchemaTest, migrates_legacy_defaults_without_extended_fields)
{
	for (uint32_t version = 1; version <= 4; ++version)
	{
		SCOPED_TRACE(version);
		auto source = PhysicsScene();
		source.version = version;
		source.entities[0].joints.clear();
		source.entities[2].character_controller.reset();
		std::string text, error;
		SceneRecord parsed;
		ASSERT_TRUE(SceneJson::Serialize(source, text, error)) << error;
		EXPECT_EQ(text.find("\"settings\""), std::string::npos);
		EXPECT_EQ(text.find("\"joints\""), std::string::npos);
		ASSERT_TRUE(SceneJson::Parse(text, parsed, error)) << error;
		EXPECT_EQ(parsed.version, CurrentSceneVersion);
		EXPECT_FALSE(parsed.entities[0].rigid_body->settings.trigger);
		EXPECT_FLOAT_EQ(parsed.entities[0].rigid_body->settings.friction, 0.5f);
	}
}

TEST(PhysicsSchemaTest, legacy_versions_reject_extended_physics)
{
	std::string error;
	auto source = PhysicsScene();
	source.version = 4;
	EXPECT_FALSE(SceneJson::Validate(source, error));
	source.entities[0].joints.clear();
	source.entities[2].character_controller.reset();
	source.entities[0].rigid_body->shape = "capsule";
	EXPECT_FALSE(SceneJson::Validate(source, error));
	source.entities[0].rigid_body->shape = "box";
	source.entities[0].rigid_body->settings.trigger = true;
	EXPECT_FALSE(SceneJson::Validate(source, error));
	SceneRecord parsed;
	EXPECT_FALSE(SceneJson::Parse(R"({"version":4,"entities":[{"id":"1","rigid_body":{"settings":{}}}]})", parsed, error));
}

TEST(PhysicsSchemaTest, rejects_dynamic_meshes_and_invalid_materials)
{
	auto source = PhysicsScene();
	std::string error;
	source.entities[0].rigid_body->shape = "triangle_mesh";
	source.entities[0].rigid_body->vertices = { { 0.f, 0.f, 0.f }, { 1.f, 0.f, 0.f }, { 0.f, 0.f, 1.f } };
	source.entities[0].rigid_body->indices = { 0, 1, 2 };
	EXPECT_FALSE(SceneJson::Validate(source, error));
	source = PhysicsScene();
	source.entities[0].rigid_body->settings.friction = -1.f;
	EXPECT_FALSE(SceneJson::Validate(source, error));
	source.entities[0].rigid_body->settings.friction = 0.5f;
	source.entities[0].rigid_body->settings.mask = 65536;
	EXPECT_FALSE(SceneJson::Validate(source, error));
}

TEST(PhysicsSchemaTest, rejects_conflicting_characters_and_scaled_ancestors)
{
	auto source = PhysicsScene();
	std::string error;
	source.entities[2].rigid_body.emplace();
	EXPECT_FALSE(SceneJson::Validate(source, error));
	source.entities[2].rigid_body.reset();
	source.entities[2].parent = "2";
	source.entities[1].transform.scale.x = 2.f;
	EXPECT_FALSE(SceneJson::Validate(source, error));
	source.entities[1].transform.scale.x = 1.f;
	source.entities[2].character_controller->max_slope = 90.f;
	EXPECT_FALSE(SceneJson::Validate(source, error));
}

TEST(PhysicsSchemaTest, rejects_missing_self_and_nondynamic_joint_targets)
{
	auto source = PhysicsScene();
	std::string error;
	source.entities[0].joints[0].target = "99";
	EXPECT_FALSE(SceneJson::Validate(source, error));
	source.entities[0].joints[0].target = "1";
	EXPECT_FALSE(SceneJson::Validate(source, error));
	source.entities[0].joints[0].target = "3";
	EXPECT_FALSE(SceneJson::Validate(source, error));
	source.entities[0].joints[0].target = "2";
	source.entities[0].rigid_body->mass = 0.f;
	EXPECT_FALSE(SceneJson::Validate(source, error));
}

TEST(PhysicsSchemaTest, rejects_joint_limits_and_collection_budgets)
{
	auto source = PhysicsScene();
	std::string error;
	source.entities[0].joints[0].lower_limit = 10.f;
	EXPECT_FALSE(SceneJson::Validate(source, error));
	source = PhysicsScene();
	source.entities[0].joints.resize(129, source.entities[0].joints[0]);
	EXPECT_FALSE(SceneJson::Validate(source, error));
	source = PhysicsScene();
	source.entities[0].rigid_body->vertices.resize(65537);
	EXPECT_FALSE(SceneJson::Validate(source, error));
}

TEST(PhysicsSceneTest, instantiates_and_captures_complete_authored_physics)
{
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	auto source = PhysicsScene();
	source.entities[0].rigid_body->shape = "capsule";
	source.entities[0].rigid_body->settings.restitution = 0.6f;
	ASSERT_TRUE(SceneSerializer::Instantiate(source, scene, {}, scripts, error)) << error;
	auto body = scene.GetEntityByUUID(UUID(1));
	auto character = scene.GetEntityByUUID(UUID(3));
	ASSERT_EQ(body.GetComponent<JointComponent>().joints.size(), 1u);
	EXPECT_TRUE(scene.GetPhysicsWorld().HasJoint(body.GetComponent<JointComponent>().joints[0].handle));
	EXPECT_TRUE(scene.GetPhysicsWorld().HasCharacter(character.GetComponent<CharacterControllerComponent>().handle));
	EXPECT_EQ(static_cast<uint64_t>(scene.GetPhysicsEntity(character.GetComponent<CharacterControllerComponent>().handle)), 3u);
	SceneRecord captured;
	ASSERT_TRUE(SceneSerializer::Capture(scene, {}, scripts, captured, error)) << error;
	const auto found = std::find_if(captured.entities.begin(), captured.entities.end(), [](const auto& entity) { return entity.id == "1"; });
	ASSERT_NE(found, captured.entities.end());
	EXPECT_EQ(found->rigid_body->shape, "capsule");
	EXPECT_FLOAT_EQ(found->rigid_body->settings.restitution, 0.6f);
	EXPECT_EQ(found->joints[0].target, "2");
	Scene reloaded;
	ASSERT_TRUE(SceneSerializer::Instantiate(captured, reloaded, {}, scripts, error)) << error;
}

TEST(PhysicsSceneTest, delivers_uuid_snapshots_and_exit_after_callback_destruction)
{
	Scene scene;
	BodyRecord trigger;
	trigger.settings.trigger = true;
	auto first = scene.CreateEntityWithUUID(UUID(1));
	first.AddComponent<RigidBodyComponent>(trigger);
	auto second = scene.CreateEntityWithUUID(UUID(2));
	second.AddComponent<RigidBodyComponent>(BodyRecord{});
	std::vector<ContactPhase> phases;
	scene.SetPhysicsEventCallback([&](const auto& event)
	{
		EXPECT_EQ(event.GetFirstEntity(), 1u);
		EXPECT_EQ(event.GetSecondEntity(), 2u);
		EXPECT_EQ(event.GetScene(), static_cast<uint64_t>(scene.GetPhysicsEventSource()));
		phases.push_back(event.GetContact().phase);
		if (event.GetContact().phase == ContactPhase::Enter)
		{
			scene.DestroyEntity(second);
			Step(scene);
		}
	});
	Step(scene, 2);
	EXPECT_EQ(phases, (std::vector<ContactPhase>{ ContactPhase::Enter, ContactPhase::Exit }));
	EXPECT_FALSE(scene.GetEntityByUUID(UUID(2)));
}

TEST(PhysicsSceneTest, event_clones_can_be_queued_without_entity_pointers)
{
	EventEngine queue;
	queue.Start();
	PhysicsContact contact;
	contact.first = 10;
	contact.second = 20;
	contact.trigger = true;
	ScenePhysicsEvent event(3, 1, 2, contact);
	ASSERT_TRUE(queue.AddEvent(event.Clone()));
	int count = 0;
	queue.DispatchPending([&](Event& base)
	{
		ASSERT_EQ(base.GetEventType(), EventType::PhysicsContact);
		EXPECT_TRUE(base.IsInCategory(PhysicsEvent));
		const auto& delivered = static_cast<const ScenePhysicsEvent&>(base);
		EXPECT_EQ(delivered.GetFirstEntity(), 1u);
		EXPECT_EQ(delivered.GetScene(), 3u);
		EXPECT_TRUE(delivered.GetContact().trigger);
		++count;
	});
	EXPECT_EQ(count, 1);
}

TEST(PhysicsSceneTest, scene_event_sources_are_distinct)
{
	Scene first, second;
	EXPECT_NE(first.GetPhysicsEventSource(), second.GetPhysicsEventSource());
}

TEST(PhysicsSceneTest, invalid_ticks_do_not_republish_contacts)
{
	Scene scene;
	BodyRecord trigger;
	trigger.settings.trigger = true;
	scene.CreateEntity().AddComponent<RigidBodyComponent>(trigger);
	scene.CreateEntity().AddComponent<RigidBodyComponent>(trigger);
	int calls = 0;
	scene.SetPhysicsEventCallback([&](const auto&) { ++calls; });
	Step(scene);
	scene.StepPhysics(DeltaTime(std::chrono::nanoseconds(0)));
	EXPECT_EQ(calls, 1);
}

TEST(PhysicsSceneTest, first_character_tick_honors_transform_edits)
{
	Scene scene;
	auto entity = scene.CreateEntity();
	CharacterSettings settings;
	settings.gravity = glm::vec3(0.f);
	entity.AddComponent<CharacterControllerComponent>(settings);
	entity.GetComponent<TransformComponent>().translation.x = 7.f;
	Step(scene);
	EXPECT_NEAR(entity.GetComponent<TransformComponent>().translation.x, 7.f, 0.001f);
}

TEST(PhysicsSceneTest, scene_gravity_round_trips_independently_of_character_gravity)
{
	auto source = PhysicsScene();
	source.gravity = glm::vec3(0.f, -3.f, 0.f);
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	ASSERT_TRUE(SceneSerializer::Instantiate(source, scene, {}, scripts, error)) << error;
	EXPECT_FLOAT_EQ(scene.GetPhysicsWorld().GetGravity().y, -3.f);
	SceneRecord captured;
	ASSERT_TRUE(SceneSerializer::Capture(scene, {}, scripts, captured, error)) << error;
	ASSERT_TRUE(captured.gravity);
	EXPECT_FLOAT_EQ(captured.gravity->y, -3.f);
	EXPECT_FLOAT_EQ(scene.GetEntityByUUID(UUID(3)).GetComponent<CharacterControllerComponent>().description.gravity.y, -9.81f);
}

TEST(PhysicsSceneTest, clear_from_event_callback_is_safe)
{
	Scene scene;
	BodyRecord trigger;
	trigger.settings.trigger = true;
	scene.CreateEntity().AddComponent<RigidBodyComponent>(trigger);
	scene.CreateEntity().AddComponent<RigidBodyComponent>(trigger);
	int calls = 0;
	scene.SetPhysicsEventCallback([&](const auto&) { ++calls; scene.Clear(); });
	Step(scene);
	EXPECT_EQ(calls, 1);
	EXPECT_TRUE(scene.IsEmpty());
}

TEST(PhysicsSceneTest, characters_use_world_poses_under_unit_scale_parents)
{
	Scene scene;
	auto parent = scene.CreateEntity();
	parent.GetComponent<TransformComponent>().translation.x = 10.f;
	auto child = scene.CreateEntity();
	child.GetComponent<TransformComponent>().translation.x = 2.f;
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	CharacterSettings settings;
	settings.gravity = glm::vec3(0.f);
	auto& controller = child.AddComponent<CharacterControllerComponent>(settings);
	controller.velocity.x = 2.f;
	Step(scene, 60);
	scene.UpdateWorldTransforms();
	EXPECT_NEAR((*scene.GetWorldTransform(child.GetUUID()))[3].x, 14.f, 0.1f);
	EXPECT_NEAR(child.GetComponent<TransformComponent>().translation.x, 4.f, 0.1f);
	parent.GetComponent<TransformComponent>().translation.x = 20.f;
	controller.velocity.x = 0.f;
	Step(scene);
	scene.UpdateWorldTransforms();
	EXPECT_NEAR((*scene.GetWorldTransform(child.GetUUID()))[3].x, 14.f, 0.1f);
}

TEST(PhysicsSceneTest, characters_reject_scaled_parenting_and_detach_runtime_violations)
{
	Scene scene;
	auto parent = scene.CreateEntity();
	auto child = scene.CreateEntity();
	CharacterSettings settings;
	settings.gravity = glm::vec3(0.f);
	child.AddComponent<CharacterControllerComponent>(settings);
	parent.GetComponent<TransformComponent>().scale.x = 2.f;
	EXPECT_FALSE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	parent.GetComponent<TransformComponent>().scale.x = 1.f;
	ASSERT_TRUE(scene.SetParent(child.GetUUID(), parent.GetUUID()));
	Step(scene);
	parent.GetComponent<TransformComponent>().scale.x = 2.f;
	Step(scene);
	EXPECT_EQ(static_cast<uint64_t>(scene.GetParent(child.GetUUID())), 0u);
}

TEST(PhysicsSceneTest, stop_releases_and_next_step_recreates_gameplay_physics)
{
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	ASSERT_TRUE(SceneSerializer::Instantiate(PhysicsScene(), scene, {}, scripts, error)) << error;
	auto body = scene.GetEntityByUUID(UUID(1));
	auto character = scene.GetEntityByUUID(UUID(3));
	const auto joint_handle = body.GetComponent<JointComponent>().joints[0].handle;
	const auto character_handle = character.GetComponent<CharacterControllerComponent>().handle;
	scene.OnRuntimeStop();
	EXPECT_FALSE(scene.GetPhysicsWorld().HasJoint(joint_handle));
	EXPECT_FALSE(scene.GetPhysicsWorld().HasCharacter(character_handle));
	Step(scene);
	EXPECT_NE(body.GetComponent<JointComponent>().joints[0].handle, joint_handle);
	EXPECT_NE(character.GetComponent<CharacterControllerComponent>().handle, character_handle);
}

TEST(PhysicsSceneTest, removing_characters_releases_ghosts)
{
	Scene scene;
	auto entity = scene.CreateEntity();
	const auto handle = entity.AddComponent<CharacterControllerComponent>().handle;
	ASSERT_NE(handle, 0u);
	entity.RemoveComponent<CharacterControllerComponent>();
	EXPECT_FALSE(scene.GetPhysicsWorld().HasCharacter(handle));
	EXPECT_EQ(static_cast<uint64_t>(scene.GetPhysicsEntity(handle)), 0u);
}

TEST(PhysicsSceneTest, destroying_joint_targets_removes_runtime_and_authored_links)
{
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	ASSERT_TRUE(SceneSerializer::Instantiate(PhysicsScene(), scene, {}, scripts, error)) << error;
	auto owner = scene.GetEntityByUUID(UUID(1));
	const auto handle = owner.GetComponent<JointComponent>().joints[0].handle;
	scene.DestroyEntity(scene.GetEntityByUUID(UUID(2)));
	EXPECT_FALSE(scene.GetPhysicsWorld().HasJoint(handle));
	EXPECT_TRUE(owner.GetComponent<JointComponent>().description.empty());
	SceneRecord captured;
	EXPECT_TRUE(SceneSerializer::Capture(scene, {}, scripts, captured, error)) << error;
}

TEST(PhysicsSceneTest, replacing_target_bodies_rebuilds_joints)
{
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	ASSERT_TRUE(SceneSerializer::Instantiate(PhysicsScene(), scene, {}, scripts, error)) << error;
	auto target = scene.GetEntityByUUID(UUID(2));
	auto owner = scene.GetEntityByUUID(UUID(1));
	const auto old_joint = owner.GetComponent<JointComponent>().joints[0].handle;
	target.RemoveComponent<RigidBodyComponent>();
	target.AddComponent<RigidBodyComponent>(BodyRecord{});
	Step(scene);
	EXPECT_FALSE(scene.GetPhysicsWorld().HasJoint(old_joint));
	const auto new_joint = owner.GetComponent<JointComponent>().joints[0].handle;
	EXPECT_NE(new_joint, old_joint);
	EXPECT_TRUE(scene.GetPhysicsWorld().HasJoint(new_joint));
}

TEST(PhysicsSceneTest, late_factory_failure_rolls_back_bodies_characters_and_joints)
{
	auto source = PhysicsScene();
	source.entities[0].scripts.push_back({ "Fail", {} });
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	RigidBodyHandle body = 0;
	CharacterHandle character = 0;
	JointHandle joint = 0;
	ASSERT_TRUE(scripts.Register<PhysicsProbeScript>("Fail",
		[](const ScriptRecord&, const EntityRecord&, std::string&) { return true; },
		[&](Entity entity, const ScriptRecord&) -> PhysicsProbeScript*
		{
			body = entity.GetComponent<RigidBodyComponent>().handle;
			joint = entity.GetComponent<JointComponent>().joints[0].handle;
			character = entity.GetScene().GetEntityByUUID(UUID(3)).GetComponent<CharacterControllerComponent>().handle;
			return nullptr;
		}, [](const PhysicsProbeScript&, ScriptRecord&) {}));
	EXPECT_FALSE(SceneSerializer::Instantiate(source, scene, {}, scripts, error));
	EXPECT_TRUE(scene.IsEmpty());
	ASSERT_NE(body, 0u);
	ASSERT_NE(character, 0u);
	ASSERT_NE(joint, 0u);
	EXPECT_FALSE(scene.GetPhysicsWorld().HasRigidBody(body));
	EXPECT_FALSE(scene.GetPhysicsWorld().HasCharacter(character));
	EXPECT_FALSE(scene.GetPhysicsWorld().HasJoint(joint));
}

TEST(PhysicsSceneTest, direct_world_removal_recreates_owned_bodies_and_joints)
{
	Scene scene;
	ScriptRegistry scripts;
	std::string error;
	ASSERT_TRUE(SceneSerializer::Instantiate(PhysicsScene(), scene, {}, scripts, error)) << error;
	auto target = scene.GetEntityByUUID(UUID(2));
	auto owner = scene.GetEntityByUUID(UUID(1));
	const auto old_body = target.GetComponent<RigidBodyComponent>().handle;
	const auto old_joint = owner.GetComponent<JointComponent>().joints[0].handle;
	scene.GetPhysicsWorld().RemoveRigidBody(old_body);
	Step(scene);
	EXPECT_NE(target.GetComponent<RigidBodyComponent>().handle, old_body);
	EXPECT_NE(owner.GetComponent<JointComponent>().joints[0].handle, old_joint);
	EXPECT_TRUE(scene.GetPhysicsWorld().HasJoint(owner.GetComponent<JointComponent>().joints[0].handle));
}

class PhysicsPrefabTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		m_root = std::filesystem::path(DL_SCENE_TEST_OUTPUT) / "physics" / ::testing::UnitTest::GetInstance()->current_test_info()->name();
		std::error_code ec;
		std::filesystem::remove_all(m_root, ec);
		std::filesystem::create_directories(m_root, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(m_files.Mount("assets", m_root, m_error));
		Write("physics.json", PhysicsScene());
	}
	void TearDown() override
	{
		std::error_code ec;
		std::filesystem::remove_all(m_root, ec);
	}
	void Write(const std::string& path, const SceneRecord& record)
	{
		std::string text;
		ASSERT_TRUE(SceneJson::Serialize(record, text, m_error)) << m_error;
		std::ofstream stream(m_root / path, std::ios::binary);
		stream << text;
		ASSERT_TRUE(stream.good());
	}
	SceneRecord Instance(const std::string& path)
	{
		SceneRecord record;
		EntityRecord anchor;
		anchor.id = "10";
		anchor.prefab = PrefabRecord{ path, { { "body", "1" }, { "target", "2" }, { "character", "3" } } };
		record.entities.push_back(anchor);
		return record;
	}
	std::filesystem::path m_root;
	VirtualFileSystem m_files;
	std::string m_error;
};

TEST_F(PhysicsPrefabTest, remaps_joint_targets_in_independent_instances)
{
	auto source = Instance("physics.json");
	auto second = source.entities[0];
	second.id = "20";
	source.entities.push_back(second);
	SceneRecord expanded;
	ASSERT_TRUE(SceneComposer::Expand(source, m_files, expanded, m_error)) << m_error;
	for (const auto& entity : expanded.entities)
		for (const auto& joint : entity.joints)
		{
			const auto target = std::find_if(expanded.entities.begin(), expanded.entities.end(), [&](const auto& candidate) { return candidate.id == joint.target; });
			ASSERT_NE(target, expanded.entities.end());
			EXPECT_EQ(target->parent, entity.parent);
		}
	EXPECT_EQ(expanded.entities.size(), 8u);
}

TEST_F(PhysicsPrefabTest, variants_and_instance_overrides_replace_physics)
{
	SceneRecord variant;
	variant.base_prefab = "physics.json";
	EntityOverrideRecord patch;
	patch.id = "3";
	patch.character_controller = CharacterSettings{};
	patch.character_controller->radius = 0.7f;
	variant.overrides.push_back(patch);
	Write("variant.json", variant);
	auto source = Instance("variant.json");
	patch.character_controller->radius = 0.8f;
	source.entities[0].prefab->overrides.push_back(patch);
	SceneRecord expanded;
	ASSERT_TRUE(SceneComposer::Expand(source, m_files, expanded, m_error)) << m_error;
	const auto found = std::find_if(expanded.entities.begin(), expanded.entities.end(), [](const auto& entity) { return entity.character_controller.has_value(); });
	ASSERT_NE(found, expanded.entities.end());
	EXPECT_FLOAT_EQ(found->character_controller->radius, 0.8f);
}

TEST_F(PhysicsPrefabTest, overrides_can_remove_characters_and_joint_collections)
{
	auto source = Instance("physics.json");
	EntityOverrideRecord character;
	character.id = "3";
	character.remove = { "character_controller" };
	EntityOverrideRecord joint;
	joint.id = "1";
	joint.joints = std::vector<JointRecord>{};
	source.entities[0].prefab->overrides = { character, joint };
	SceneRecord expanded;
	ASSERT_TRUE(SceneComposer::Expand(source, m_files, expanded, m_error)) << m_error;
	for (const auto& entity : expanded.entities)
	{
		EXPECT_FALSE(entity.character_controller);
		EXPECT_TRUE(entity.joints.empty());
	}
}

TEST_F(PhysicsPrefabTest, invalid_overrides_fail_before_destination_mutation)
{
	Scene scene;
	scene.CreateEntityWithUUID(UUID(100), "Existing");
	auto prefab = *Instance("physics.json").entities[0].prefab;
	EntityOverrideRecord patch;
	patch.id = "2";
	patch.remove = { "rigid_body" };
	prefab.overrides.push_back(patch);
	ScriptRegistry scripts;
	UUID instance(7);
	EXPECT_FALSE(SceneSerializer::SpawnPrefab(prefab, scene, m_root, scripts, instance, m_error));
	EXPECT_EQ(static_cast<uint64_t>(instance), 7u);
	EXPECT_TRUE(scene.GetEntityByUUID(UUID(100)));
	EXPECT_FALSE(scene.FindEntityByName("Physics"));
}

TEST_F(PhysicsPrefabTest, spawned_joints_and_characters_remap_and_save_expanded)
{
	Scene scene;
	scene.CreateEntityWithUUID(UUID(1), "Existing");
	ScriptRegistry scripts;
	UUID instance(0);
	TransformRecord transform;
	transform.translation.x = 20.f;
	ASSERT_TRUE(SceneSerializer::SpawnPrefab(*Instance("physics.json").entities[0].prefab, scene, m_root, scripts, instance, m_error, transform)) << m_error;
	auto anchor = scene.GetEntityByUUID(instance);
	auto body = anchor.GetLinkedEntity("body");
	auto target = anchor.GetLinkedEntity("target");
	auto character = anchor.GetLinkedEntity("character");
	ASSERT_TRUE(body && target && character);
	EXPECT_EQ(body.GetComponent<JointComponent>().description[0].target, std::to_string(static_cast<uint64_t>(target.GetUUID())));
	EXPECT_TRUE(scene.GetPhysicsWorld().HasJoint(body.GetComponent<JointComponent>().joints[0].handle));
	glm::vec3 position;
	ASSERT_TRUE(scene.GetPhysicsWorld().GetCharacterPosition(character.GetComponent<CharacterControllerComponent>().handle, position));
	EXPECT_NEAR(position.x, 30.f, 0.001f);
	SceneRecord captured;
	ASSERT_TRUE(SceneSerializer::Capture(scene, m_root, scripts, captured, m_error)) << m_error;
	for (const auto& entity : captured.entities) EXPECT_FALSE(entity.prefab);
	Scene reloaded;
	ASSERT_TRUE(SceneSerializer::Instantiate(captured, reloaded, m_root, scripts, m_error)) << m_error;
}

TEST_F(PhysicsPrefabTest, nested_prefabs_remap_joint_targets_twice)
{
	Write("nested.json", Instance("physics.json"));
	SceneRecord expanded;
	ASSERT_TRUE(SceneComposer::Expand(Instance("nested.json"), m_files, expanded, m_error)) << m_error;
	Scene scene;
	ScriptRegistry scripts;
	ASSERT_TRUE(SceneSerializer::Instantiate(expanded, scene, m_root, scripts, m_error)) << m_error;
	SceneRecord captured;
	ASSERT_TRUE(SceneSerializer::Capture(scene, m_root, scripts, captured, m_error)) << m_error;
}

TEST_F(PhysicsPrefabTest, physics_prefabs_load_from_zip_mounts)
{
	std::string text;
	ASSERT_TRUE(SceneJson::Serialize(PhysicsScene(), text, m_error));
	int error = 0;
	auto* archive = zip_open((m_root / "physics.zip").string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error);
	ASSERT_NE(archive, nullptr);
	auto* source = zip_source_buffer(archive, text.data(), text.size(), 0);
	ASSERT_NE(source, nullptr);
	ASSERT_GE(zip_file_add(archive, "physics.json", source, ZIP_FL_ENC_UTF_8), 0);
	ASSERT_EQ(zip_close(archive), 0);
	VirtualFileSystem files;
	ASSERT_TRUE(files.Mount("assets", m_root / "physics.zip", m_error)) << m_error;
	SceneRecord expanded;
	ASSERT_TRUE(SceneComposer::Expand(Instance("physics.json"), files, expanded, m_error)) << m_error;
	EXPECT_EQ(expanded.entities.size(), 4u);
}

TEST_F(PhysicsPrefabTest, spawning_preserves_destination_gravity)
{
	auto source = PhysicsScene();
	source.gravity = glm::vec3(0.f, -20.f, 0.f);
	Write("physics.json", source);
	Scene scene;
	scene.GetPhysicsWorld().SetGravity({ 0.f, -3.f, 0.f });
	ScriptRegistry scripts;
	UUID instance(0);
	ASSERT_TRUE(SceneSerializer::SpawnPrefab(*Instance("physics.json").entities[0].prefab, scene, m_root, scripts, instance, m_error)) << m_error;
	EXPECT_FLOAT_EQ(scene.GetPhysicsWorld().GetGravity().y, -3.f);
}

TEST_F(PhysicsPrefabTest, typed_overrides_replace_joint_and_body_settings)
{
	auto source = Instance("physics.json");
	EntityOverrideRecord patch;
	patch.id = "1";
	patch.rigid_body = PhysicsScene().entities[0].rigid_body;
	patch.rigid_body->settings.trigger = true;
	patch.joints = PhysicsScene().entities[0].joints;
	(*patch.joints)[0].type = "slider";
	(*patch.joints)[0].upper_limit = 2.f;
	source.entities[0].prefab->overrides.push_back(patch);
	SceneRecord expanded;
	ASSERT_TRUE(SceneComposer::Expand(source, m_files, expanded, m_error)) << m_error;
	const auto found = std::find_if(expanded.entities.begin(), expanded.entities.end(), [](const auto& entity) { return !entity.joints.empty(); });
	ASSERT_NE(found, expanded.entities.end());
	EXPECT_EQ(found->joints[0].type, "slider");
	EXPECT_FLOAT_EQ(found->joints[0].upper_limit, 2.f);
	EXPECT_TRUE(found->rigid_body->settings.trigger);
}

TEST_F(PhysicsPrefabTest, spawning_does_not_initialize_existing_stopped_joints)
{
	Scene scene;
	ScriptRegistry scripts;
	ASSERT_TRUE(SceneSerializer::Instantiate(PhysicsScene(), scene, {}, scripts, m_error)) << m_error;
	auto owner = scene.GetEntityByUUID(UUID(1));
	scene.OnRuntimeStop();
	ASSERT_TRUE(owner.GetComponent<JointComponent>().joints.empty());
	UUID instance(0);
	ASSERT_TRUE(SceneSerializer::SpawnPrefab(*Instance("physics.json").entities[0].prefab, scene, m_root, scripts, instance, m_error)) << m_error;
	EXPECT_TRUE(owner.GetComponent<JointComponent>().joints.empty());
	EXPECT_EQ(owner.GetComponent<RigidBodyComponent>().handle, 0u);
}
}
