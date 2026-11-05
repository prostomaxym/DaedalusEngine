#include <Physics/PhysicsWorld.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace Daedalus
{
namespace
{
	ColliderDescription Collider(const std::string& shape)
	{
		ColliderDescription result;
		result.shape = shape;
		if (shape == "convex_hull") result.vertices = { { -1.f, -1.f, -1.f }, { 1.f, -1.f, -1.f }, { 0.f, 1.f, -1.f }, { 0.f, 0.f, 1.f } };
		if (shape == "triangle_mesh")
		{
			result.vertices = { { -5.f, 0.f, -5.f }, { 5.f, 0.f, -5.f }, { 0.f, 0.f, 5.f } };
			result.indices = { 0, 2, 1 };
		}
		return result;
	}

	void Tick(PhysicsWorld& world, int count = 60)
	{
		for (int i = 0; i < count; ++i) world.StepSimulation(1.f / 60.f);
	}

	bool Finite(const glm::vec3& value)
	{
		return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
	}
}

TEST(PhysicsColliderTest, creates_every_supported_shape)
{
	PhysicsWorld world(glm::vec3(0.f));
	for (const auto* shape : { "box", "sphere", "capsule", "cylinder", "cone", "convex_hull", "triangle_mesh" })
	{
		SCOPED_TRACE(shape);
		const auto collider = Collider(shape);
		ASSERT_TRUE(PhysicsWorld::ValidateCollider(collider));
		const auto body = world.CreateRigidBody(collider, 0.f, glm::vec3(0.f));
		ASSERT_NE(body, 0u);
		EXPECT_TRUE(world.HasRigidBody(body));
		EXPECT_TRUE(world.Raycast({ 0.f, 5.f, 0.f }, { 0.f, -5.f, 0.f }));
		world.RemoveRigidBody(body);
		EXPECT_FALSE(world.HasRigidBody(body));
	}
}

TEST(PhysicsColliderTest, rejects_dynamic_triangle_meshes)
{
	PhysicsWorld world;
	EXPECT_EQ(world.CreateRigidBody(Collider("triangle_mesh"), 1.f, glm::vec3(0.f)), 0u);
	EXPECT_NE(world.CreateRigidBody(Collider("triangle_mesh"), 0.f, glm::vec3(0.f), glm::quat(1.f, 0.f, 0.f, 0.f), true), 0u);
}

TEST(PhysicsColliderTest, rejects_invalid_geometry)
{
	auto collider = Collider("box");
	collider.half_extents.x = 0.f;
	EXPECT_FALSE(PhysicsWorld::ValidateCollider(collider));
	collider = Collider("capsule");
	collider.height = -1.f;
	EXPECT_FALSE(PhysicsWorld::ValidateCollider(collider));
	collider = Collider("cylinder");
	collider.half_extents.z = 1.f;
	EXPECT_FALSE(PhysicsWorld::ValidateCollider(collider));
	collider = Collider("convex_hull");
	collider.vertices.back().z = -1.f;
	EXPECT_FALSE(PhysicsWorld::ValidateCollider(collider));
	collider = Collider("triangle_mesh");
	collider.indices.back() = 50;
	EXPECT_FALSE(PhysicsWorld::ValidateCollider(collider));
	collider.indices = { 0, 0, 1 };
	EXPECT_FALSE(PhysicsWorld::ValidateCollider(collider));
	collider = Collider("unknown");
	EXPECT_FALSE(PhysicsWorld::ValidateCollider(collider));
}

TEST(PhysicsColliderTest, rejects_nonfinite_values_and_geometry_budgets)
{
	auto collider = Collider("sphere");
	collider.radius = std::numeric_limits<float>::infinity();
	EXPECT_FALSE(PhysicsWorld::ValidateCollider(collider));
	collider = Collider("convex_hull");
	collider.vertices.resize(65537);
	EXPECT_FALSE(PhysicsWorld::ValidateCollider(collider));
	collider = Collider("triangle_mesh");
	collider.indices.resize(196609);
	EXPECT_FALSE(PhysicsWorld::ValidateCollider(collider));
	PhysicsWorld world;
	EXPECT_EQ(world.CreateRigidBody(Collider("box"), -1.f, glm::vec3(0.f)), 0u);
	EXPECT_EQ(world.CreateRigidBody(Collider("box"), 1.f, glm::vec3(0.f), glm::quat(0.f, 0.f, 0.f, 0.f)), 0u);
}

TEST(PhysicsBodySettingsTest, validates_filters_and_material_ranges)
{
	PhysicsBodySettings settings;
	EXPECT_TRUE(PhysicsWorld::ValidateBodySettings(settings));
	settings.group = 0;
	EXPECT_FALSE(PhysicsWorld::ValidateBodySettings(settings));
	settings.group = 1;
	settings.mask = 65536;
	EXPECT_FALSE(PhysicsWorld::ValidateBodySettings(settings));
	settings.mask = 65535;
	settings.restitution = 1.1f;
	EXPECT_FALSE(PhysicsWorld::ValidateBodySettings(settings));
	settings.restitution = 0.7f;
	settings.linear_damping = -0.1f;
	EXPECT_FALSE(PhysicsWorld::ValidateBodySettings(settings));
	PhysicsWorld world;
	EXPECT_EQ(world.CreateRigidBody(Collider("box"), 1.f, glm::vec3(0.f), glm::quat(1.f, 0.f, 0.f, 0.f), false, settings), 0u);
}

TEST(PhysicsBodySettingsTest, applies_velocity_and_mass_scaled_impulse)
{
	PhysicsWorld world(glm::vec3(0.f));
	const auto body = world.CreateRigidBody(Collider("sphere"), 2.f, glm::vec3(0.f));
	ASSERT_TRUE(world.SetLinearVelocity(body, { 1.f, 0.f, 0.f }));
	ASSERT_TRUE(world.ApplyImpulse(body, { 2.f, 0.f, 0.f }));
	glm::vec3 velocity;
	ASSERT_TRUE(world.GetLinearVelocity(body, velocity));
	EXPECT_NEAR(velocity.x, 2.f, 0.001f);
	Tick(world);
	glm::vec3 position;
	glm::quat rotation;
	ASSERT_TRUE(world.GetRigidBodyTransform(body, position, rotation));
	EXPECT_NEAR(position.x, 2.f, 0.01f);
	const auto fixed = world.CreateRigidBody(Collider("box"), 0.f, { 10.f, 0.f, 0.f });
	EXPECT_FALSE(world.SetLinearVelocity(fixed, glm::vec3(1.f)));
	EXPECT_FALSE(world.ApplyImpulse(fixed, glm::vec3(1.f)));
	EXPECT_FALSE(world.ApplyImpulse(0, glm::vec3(1.f)));
}

TEST(PhysicsQueryTest, raycast_returns_closest_world_hit)
{
	PhysicsWorld world;
	const auto first = world.CreateRigidBody(Collider("box"), 0.f, glm::vec3(0.f));
	world.CreateRigidBody(Collider("sphere"), 0.f, { 3.f, 0.f, 0.f });
	const auto hit = world.Raycast({ -5.f, 0.f, 0.f }, { 5.f, 0.f, 0.f });
	ASSERT_TRUE(hit);
	EXPECT_EQ(hit->body, first);
	EXPECT_NEAR(hit->fraction, 0.45f, 0.002f);
	EXPECT_NEAR(hit->position.x, -0.5f, 0.01f);
	EXPECT_NEAR(hit->normal.x, -1.f, 0.01f);
	EXPECT_FALSE(world.Raycast(glm::vec3(0.f), glm::vec3(0.f)));
}

TEST(PhysicsQueryTest, filters_bilaterally_and_ignores_handles)
{
	PhysicsWorld world;
	PhysicsBodySettings settings;
	settings.group = 2;
	settings.mask = 4;
	const auto body = world.CreateRigidBody(Collider("box"), 0.f, glm::vec3(0.f), glm::quat(1.f, 0.f, 0.f, 0.f), false, settings);
	EXPECT_FALSE(world.Raycast({ -5.f, 0.f, 0.f }, { 5.f, 0.f, 0.f }));
	PhysicsQueryFilter filter;
	filter.group = 4;
	filter.mask = 2;
	ASSERT_TRUE(world.Raycast({ -5.f, 0.f, 0.f }, { 5.f, 0.f, 0.f }, filter));
	filter.ignore = body;
	EXPECT_FALSE(world.Raycast({ -5.f, 0.f, 0.f }, { 5.f, 0.f, 0.f }, filter));
	EXPECT_TRUE(world.Overlap(Collider("sphere"), glm::vec3(0.f), glm::quat(1.f, 0.f, 0.f, 0.f), filter).empty());
}

TEST(PhysicsQueryTest, excludes_triggers_unless_requested)
{
	PhysicsWorld world;
	PhysicsBodySettings settings;
	settings.trigger = true;
	const auto body = world.CreateRigidBody(Collider("box"), 0.f, glm::vec3(0.f), glm::quat(1.f, 0.f, 0.f, 0.f), false, settings);
	EXPECT_FALSE(world.Raycast({ -5.f, 0.f, 0.f }, { 5.f, 0.f, 0.f }));
	PhysicsQueryFilter filter;
	filter.include_triggers = true;
	const auto hit = world.Raycast({ -5.f, 0.f, 0.f }, { 5.f, 0.f, 0.f }, filter);
	ASSERT_TRUE(hit);
	EXPECT_EQ(hit->body, body);
	EXPECT_EQ(world.Overlap(Collider("sphere"), glm::vec3(0.f), glm::quat(1.f, 0.f, 0.f, 0.f), filter), std::vector<RigidBodyHandle>{ body });
}

TEST(PhysicsQueryTest, convex_sweep_returns_earlier_surface_hit)
{
	PhysicsWorld world;
	const auto body = world.CreateRigidBody(Collider("box"), 0.f, glm::vec3(0.f));
	const auto hit = world.Sweep(Collider("sphere"), { -5.f, 0.f, 0.f }, { 5.f, 0.f, 0.f });
	ASSERT_TRUE(hit);
	EXPECT_EQ(hit->body, body);
	EXPECT_NEAR(hit->fraction, 0.4f, 0.01f);
	EXPECT_FALSE(world.Sweep(Collider("triangle_mesh"), { -5.f, 0.f, 0.f }, { 5.f, 0.f, 0.f }));
}

TEST(PhysicsQueryTest, overlap_uses_contact_geometry_not_only_aabbs)
{
	PhysicsWorld world;
	auto sphere = Collider("sphere");
	sphere.radius = 1.f;
	const auto body = world.CreateRigidBody(sphere, 0.f, glm::vec3(0.f));
	EXPECT_TRUE(world.Overlap(sphere, { 1.5f, 1.5f, 0.f }).empty());
	EXPECT_EQ(world.Overlap(sphere, { 1.5f, 0.f, 0.f }), std::vector<RigidBodyHandle>{ body });
	ASSERT_TRUE(world.SetRigidBodyTransform(body, { 10.f, 0.f, 0.f }, glm::quat(1.f, 0.f, 0.f, 0.f)));
	EXPECT_TRUE(world.Overlap(sphere, glm::vec3(0.f)).empty());
}

TEST(PhysicsContactTest, reports_enter_stay_exit_for_stationary_triggers)
{
	PhysicsWorld world(glm::vec3(0.f));
	PhysicsBodySettings settings;
	settings.trigger = true;
	const auto first = world.CreateRigidBody(Collider("box"), 0.f, glm::vec3(0.f), glm::quat(1.f, 0.f, 0.f, 0.f), false, settings);
	const auto second = world.CreateRigidBody(Collider("box"), 0.f, { 0.5f, 0.f, 0.f });
	int legacy_calls = 0;
	world.RegisterCollisionCallback([&](auto, auto) { ++legacy_calls; });
	world.StepSimulation();
	ASSERT_EQ(world.GetContactEvents().size(), 1u);
	EXPECT_EQ(world.GetContactEvents()[0].phase, ContactPhase::Enter);
	EXPECT_EQ(world.GetContactEvents()[0].first, first);
	EXPECT_EQ(world.GetContactEvents()[0].second, second);
	EXPECT_TRUE(world.GetContactEvents()[0].trigger);
	EXPECT_GT(world.GetContactEvents()[0].normal.x, 0.f);
	world.StepSimulation();
	ASSERT_EQ(world.GetContactEvents().size(), 1u);
	EXPECT_EQ(world.GetContactEvents()[0].phase, ContactPhase::Stay);
	world.RemoveRigidBody(second);
	world.StepSimulation();
	ASSERT_EQ(world.GetContactEvents().size(), 1u);
	EXPECT_EQ(world.GetContactEvents()[0].phase, ContactPhase::Exit);
	world.StepSimulation();
	EXPECT_TRUE(world.GetContactEvents().empty());
	EXPECT_EQ(legacy_calls, 0);
}

TEST(PhysicsContactTest, preserves_legacy_callbacks_for_solid_contacts)
{
	PhysicsWorld world(glm::vec3(0.f));
	world.CreateBoxRigidBody(0.f, glm::vec3(1.f), { 1.f, 0.f, 0.f });
	world.CreateBoxRigidBody(1.f, glm::vec3(1.f), glm::vec3(0.f));
	int calls = 0;
	world.RegisterCollisionCallback([&](auto, auto) { ++calls; });
	Tick(world, 2);
	EXPECT_EQ(calls, 2);
	ASSERT_EQ(world.GetContactEvents().size(), 1u);
	EXPECT_FALSE(world.GetContactEvents()[0].trigger);
	EXPECT_EQ(world.GetContactEvents()[0].phase, ContactPhase::Stay);
}

TEST(PhysicsContactTest, snapshots_allow_removal_and_reentrant_step_attempts)
{
	PhysicsWorld world(glm::vec3(0.f));
	PhysicsBodySettings settings;
	settings.trigger = true;
	const auto first = world.CreateRigidBody(Collider("box"), 0.f, glm::vec3(0.f), glm::quat(1.f, 0.f, 0.f, 0.f), false, settings);
	world.CreateRigidBody(Collider("box"), 0.f, glm::vec3(0.f), glm::quat(1.f, 0.f, 0.f, 0.f), false, settings);
	world.CreateRigidBody(Collider("box"), 0.f, glm::vec3(0.f), glm::quat(1.f, 0.f, 0.f, 0.f), false, settings);
	int calls = 0;
	world.RegisterContactCallback([&](const auto&)
	{
		++calls;
		world.RemoveRigidBody(first);
		world.StepSimulation();
		world.RegisterContactCallback({});
	});
	world.StepSimulation();
	EXPECT_EQ(calls, 3);
	world.StepSimulation();
	EXPECT_EQ(std::count_if(world.GetContactEvents().begin(), world.GetContactEvents().end(), [](const auto& event) { return event.phase == ContactPhase::Exit; }), 2);
}

TEST(PhysicsContactTest, trigger_volumes_do_not_stop_falling_bodies)
{
	PhysicsWorld world;
	PhysicsBodySettings settings;
	settings.trigger = true;
	world.CreateRigidBody(Collider("box"), 0.f, glm::vec3(0.f), glm::quat(1.f, 0.f, 0.f, 0.f), false, settings);
	const auto falling = world.CreateRigidBody(Collider("sphere"), 1.f, { 0.f, 3.f, 0.f });
	int enters = 0, exits = 0;
	world.RegisterContactCallback([&](const auto& event)
	{
		if (event.trigger && event.phase == ContactPhase::Enter) ++enters;
		if (event.trigger && event.phase == ContactPhase::Exit) ++exits;
	});
	Tick(world, 120);
	glm::vec3 position;
	glm::quat rotation;
	ASSERT_TRUE(world.GetRigidBodyTransform(falling, position, rotation));
	EXPECT_LT(position.y, -5.f);
	EXPECT_EQ(enters, 1);
	EXPECT_EQ(exits, 1);
}

TEST(PhysicsContactTest, honors_collision_masks_for_static_trigger_tests)
{
	PhysicsWorld world;
	PhysicsBodySettings settings;
	settings.trigger = true;
	settings.mask = 1;
	world.CreateRigidBody(Collider("box"), 0.f, glm::vec3(0.f), glm::quat(1.f, 0.f, 0.f, 0.f), false, settings);
	settings.group = 2;
	settings.mask = 2;
	world.CreateRigidBody(Collider("box"), 0.f, glm::vec3(0.f), glm::quat(1.f, 0.f, 0.f, 0.f), false, settings);
	world.StepSimulation();
	EXPECT_TRUE(world.GetContactEvents().empty());
}

TEST(PhysicsCharacterTest, settles_moves_and_jumps)
{
	PhysicsWorld world;
	world.CreateBoxRigidBody(0.f, { 10.f, 0.5f, 10.f }, { 0.f, -0.5f, 0.f });
	const auto character = world.CreateCharacter({}, { 0.f, 3.f, 0.f });
	ASSERT_NE(character, 0u);
	Tick(world, 180);
	glm::vec3 position;
	ASSERT_TRUE(world.GetCharacterPosition(character, position));
	EXPECT_NEAR(position.y, 1.f, 0.15f);
	ASSERT_TRUE(world.IsCharacterGrounded(character));
	ASSERT_TRUE(world.SetCharacterVelocity(character, { 2.f, 0.f, 0.f }));
	Tick(world);
	ASSERT_TRUE(world.GetCharacterPosition(character, position));
	EXPECT_NEAR(position.x, 2.f, 0.1f);
	const auto standing_height = position.y;
	ASSERT_TRUE(world.SetCharacterVelocity(character, glm::vec3(0.f)));
	ASSERT_TRUE(world.JumpCharacter(character));
	Tick(world, 20);
	ASSERT_TRUE(world.GetCharacterPosition(character, position));
	EXPECT_GT(position.y, standing_height + 0.5f);
	EXPECT_FALSE(world.JumpCharacter(character));
}

TEST(PhysicsCharacterTest, stops_at_walls)
{
	PhysicsWorld world;
	world.CreateBoxRigidBody(0.f, { 10.f, 0.5f, 10.f }, { 0.f, -0.5f, 0.f });
	world.CreateBoxRigidBody(0.f, { 0.5f, 5.f, 5.f }, { 2.5f, 5.f, 0.f });
	const auto character = world.CreateCharacter({}, { 0.f, 1.1f, 0.f });
	ASSERT_TRUE(world.SetCharacterVelocity(character, { 2.f, 0.f, 0.f }));
	Tick(world, 180);
	glm::vec3 position;
	ASSERT_TRUE(world.GetCharacterPosition(character, position));
	EXPECT_GT(position.x, 1.f);
	EXPECT_LT(position.x, 1.6f);
}

TEST(PhysicsCharacterTest, climbs_steps_below_step_height)
{
	PhysicsWorld world;
	world.CreateBoxRigidBody(0.f, { 10.f, 0.5f, 10.f }, { 0.f, -0.5f, 0.f });
	world.CreateBoxRigidBody(0.f, { 0.5f, 0.1f, 2.f }, { 1.5f, 0.1f, 0.f });
	const auto character = world.CreateCharacter({}, { 0.f, 1.1f, 0.f });
	ASSERT_TRUE(world.SetCharacterVelocity(character, { 2.f, 0.f, 0.f }));
	Tick(world, 120);
	glm::vec3 position;
	ASSERT_TRUE(world.GetCharacterPosition(character, position));
	EXPECT_GT(position.x, 3.f);
}

TEST(PhysicsCharacterTest, warp_updates_queries_and_cleanup)
{
	PhysicsWorld world;
	const auto character = world.CreateCharacter({}, { 0.f, 2.f, 0.f });
	EXPECT_FALSE(world.IsCharacterGrounded(character));
	EXPECT_FALSE(world.JumpCharacter(character));
	ASSERT_TRUE(world.WarpCharacter(character, { 10.f, 2.f, 0.f }));
	EXPECT_FALSE(world.IsCharacterGrounded(character));
	EXPECT_FALSE(world.JumpCharacter(character));
	const auto hit = world.Raycast({ 5.f, 2.f, 0.f }, { 15.f, 2.f, 0.f });
	ASSERT_TRUE(hit);
	EXPECT_EQ(hit->body, character);
	EXPECT_FALSE(world.SetCharacterVelocity(character, { 0.f, 1.f, 0.f }));
	const auto cleanup = world.MakeCharacterCleanup(character);
	cleanup();
	cleanup();
	EXPECT_FALSE(world.HasCharacter(character));
	glm::vec3 position;
	EXPECT_FALSE(world.GetCharacterPosition(character, position));
}

TEST(PhysicsCharacterTest, rejects_invalid_settings)
{
	PhysicsWorld world;
	CharacterSettings settings;
	settings.max_slope = 90.f;
	EXPECT_EQ(world.CreateCharacter(settings, glm::vec3(0.f)), 0u);
	settings = {};
	settings.gravity.x = 1.f;
	EXPECT_FALSE(PhysicsWorld::ValidateCharacterSettings(settings));
	settings = {};
	settings.radius = 0.f;
	EXPECT_FALSE(PhysicsWorld::ValidateCharacterSettings(settings));
	EXPECT_FALSE(world.JumpCharacter(0));
}

TEST(PhysicsCharacterTest, initial_capsule_is_upright_before_first_tick)
{
	PhysicsWorld world;
	const auto character = world.CreateCharacter({}, { 0.f, 2.f, 0.f });
	const auto hit = world.Raycast({ -2.f, 2.8f, 0.f }, { 2.f, 2.8f, 0.f });
	ASSERT_TRUE(hit);
	EXPECT_EQ(hit->body, character);
}

TEST(PhysicsContactTest, trigger_tests_respect_joint_collision_suppression)
{
	PhysicsWorld world(glm::vec3(0.f));
	PhysicsBodySettings settings;
	settings.trigger = true;
	const auto first = world.CreateRigidBody(Collider("box"), 0.f, glm::vec3(0.f), glm::quat(1.f, 0.f, 0.f, 0.f), false, settings);
	const auto second = world.CreateRigidBody(Collider("box"), 1.f, glm::vec3(0.f));
	ASSERT_NE(world.CreateJoint(first, second, {}), 0u);
	world.StepSimulation();
	EXPECT_TRUE(world.GetContactEvents().empty());
}

TEST(PhysicsJointTest, creates_and_constrains_every_joint_type)
{
	for (const auto* type : { "fixed", "point", "hinge", "slider", "cone" })
	{
		SCOPED_TRACE(type);
		PhysicsWorld world(glm::vec3(0.f));
		const auto first = world.CreateSphereRigidBody(0.f, 0.5f, glm::vec3(0.f));
		const auto second = world.CreateSphereRigidBody(1.f, 0.5f, { 2.f, 0.f, 0.f });
		JointSettings settings;
		settings.type = type;
		settings.pivot_a.x = 2.f;
		settings.lower_limit = -1.f;
		settings.upper_limit = 1.f;
		const auto joint = world.CreateJoint(first, second, settings);
		ASSERT_NE(joint, 0u);
		ASSERT_TRUE(world.ApplyImpulse(second, { 10.f, 0.f, 0.f }));
		Tick(world, 120);
		glm::vec3 position;
		glm::quat rotation;
		ASSERT_TRUE(world.GetRigidBodyTransform(second, position, rotation));
		EXPECT_NEAR(position.x, 2.f, settings.type == "slider" ? 1.05f : 0.05f);
		EXPECT_TRUE(world.HasJoint(joint));
		world.RemoveRigidBody(first);
		EXPECT_FALSE(world.HasJoint(joint));
		world.RemoveJoint(joint);
	}
}

TEST(PhysicsJointTest, rejects_invalid_targets_and_limits)
{
	PhysicsWorld world;
	const auto first = world.CreateBoxRigidBody(0.f, glm::vec3(1.f), glm::vec3(0.f));
	const auto second = world.CreateBoxRigidBody(0.f, glm::vec3(1.f), glm::vec3(3.f));
	EXPECT_EQ(world.CreateJoint(first, second, {}), 0u);
	EXPECT_EQ(world.CreateJoint(first, first, {}), 0u);
	EXPECT_EQ(world.CreateJoint(first, 0, {}), 0u);
	JointSettings settings;
	settings.lower_limit = 1.f;
	EXPECT_FALSE(PhysicsWorld::ValidateJointSettings(settings));
	settings = {};
	settings.swing_limits.y = 181.f;
	EXPECT_FALSE(PhysicsWorld::ValidateJointSettings(settings));
}

TEST(PhysicsJointTest, break_threshold_disables_constraint)
{
	PhysicsWorld world(glm::vec3(0.f));
	const auto first = world.CreateBoxRigidBody(0.f, glm::vec3(1.f), glm::vec3(0.f));
	const auto second = world.CreateBoxRigidBody(1.f, glm::vec3(1.f), { 3.f, 0.f, 0.f });
	JointSettings settings;
	settings.pivot_a.x = 3.f;
	settings.breaking_impulse = 0.01f;
	const auto joint = world.CreateJoint(first, second, settings);
	ASSERT_NE(joint, 0u);
	ASSERT_TRUE(world.ApplyImpulse(second, { 100.f, 0.f, 0.f }));
	Tick(world, 2);
	EXPECT_TRUE(world.HasJoint(joint));
	EXPECT_FALSE(world.IsJointEnabled(joint));
}

TEST(PhysicsLifetimeTest, cleanup_callbacks_do_not_keep_world_alive)
{
	std::function<void()> body_cleanup, character_cleanup, joint_cleanup;
	{
		PhysicsWorld world;
		const auto first = world.CreateBoxRigidBody(0.f, glm::vec3(1.f), glm::vec3(0.f));
		const auto second = world.CreateBoxRigidBody(1.f, glm::vec3(1.f), { 3.f, 0.f, 0.f });
		body_cleanup = world.MakeBodyCleanup(first);
		character_cleanup = world.MakeCharacterCleanup(world.CreateCharacter({}, { 10.f, 0.f, 0.f }));
		joint_cleanup = world.MakeJointCleanup(world.CreateJoint(first, second, {}));
	}
	body_cleanup();
	character_cleanup();
	joint_cleanup();
}

TEST(PhysicsDebugDrawTest, emits_finite_headless_geometry)
{
	PhysicsWorld world;
	world.CreateRigidBody(Collider("box"), 0.f, glm::vec3(0.f));
	world.CreateCharacter({}, { 3.f, 2.f, 0.f });
	size_t lines = 0;
	world.DrawDebug([&](const auto& start, const auto& end, const auto& color)
	{
		++lines;
		EXPECT_TRUE(Finite(start));
		EXPECT_TRUE(Finite(end));
		EXPECT_TRUE(Finite(color));
	});
	EXPECT_GT(lines, 12u);
	EXPECT_LE(lines, 65536u);
	world.DrawDebug({});
}
}
