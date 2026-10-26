#include <Macros.h>
#include <Physics/Physics.h>

#include <gtest/gtest.h>

using namespace Daedalus;

class PhysicsTest : public ::testing::Test
{
protected:
	PhysicsWorld m_world;

	glm::vec3 PositionOf(RigidBodyHandle body)
	{
		glm::vec3 position;
		glm::quat orientation;
		EXPECT_TRUE(m_world.GetRigidBodyTransform(body, position, orientation));
		return position;
	}

	void Simulate(float seconds)
	{
		constexpr float step = 1.f / 60.f;
		for (float elapsed = 0.f; elapsed < seconds; elapsed += step)
		{
			m_world.StepSimulation(step);
		}
	}
};

TEST(PhysicsWorldTest, worlds_reject_foreign_handles)
{
	PhysicsWorld first, second;

	const auto handle = first.CreateSphereRigidBody(1.f, 0.5f, glm::vec3(0.f, 3.f, 0.f));

	glm::vec3 pos;
	glm::quat orientation;
	EXPECT_FALSE(second.GetRigidBodyTransform(handle, pos, orientation));

	second.RemoveRigidBody(handle);
	EXPECT_TRUE(first.GetRigidBodyTransform(handle, pos, orientation));

}

TEST(PhysicsWorldTest, removed_handles_cannot_alias_new_bodies)
{
	PhysicsWorld world;
	const auto old = world.CreateSphereRigidBody(1.f, 1.f, glm::vec3(0.f));
	world.RemoveRigidBody(old);
	const auto replacement = world.CreateSphereRigidBody(1.f, 1.f, glm::vec3(0.f));
	EXPECT_NE(old, replacement);
	glm::vec3 position;
	glm::quat orientation;
	EXPECT_FALSE(world.GetRigidBodyTransform(old, position, orientation));
	world.RemoveRigidBody(old);
	EXPECT_TRUE(world.GetRigidBodyTransform(replacement, position, orientation));
}

TEST(PhysicsWorldTest, cleanup_is_safe_after_world_destruction)
{
	std::function<void()> cleanup;
	{
		PhysicsWorld world;
		cleanup = world.MakeBodyCleanup(world.CreateSphereRigidBody(1.f, 1.f, glm::vec3(0.f)));
	}
	cleanup();
	cleanup();
}

TEST(PhysicsWorldTest, collision_callback_can_remove_a_body)
{
	PhysicsWorld world;
	world.CreateBoxRigidBody(0.f, glm::vec3(10.f, 0.5f, 10.f), glm::vec3(0.f));
	const auto ball = world.CreateSphereRigidBody(1.f, 0.5f, glm::vec3(0.f, 0.8f, 0.f));
	bool called = false;
	world.RegisterCollisionCallback([&](RigidBodyHandle, RigidBodyHandle)
	{
		called = true;
		world.RemoveRigidBody(ball);
		world.RegisterCollisionCallback({});
	});
	world.StepSimulation();
	EXPECT_TRUE(called);
	glm::vec3 position;
	glm::quat orientation;
	EXPECT_FALSE(world.GetRigidBodyTransform(ball, position, orientation));
}

TEST_F(PhysicsTest, created_bodies_have_non_null_handles)
{
	EXPECT_NE(m_world.CreateBoxRigidBody(1.f, glm::vec3(0.5f), glm::vec3(0.f)), 0u);
	EXPECT_NE(m_world.CreateSphereRigidBody(1.f, 0.5f, glm::vec3(0.f)), 0u);
}

TEST_F(PhysicsTest, created_body_reports_initial_position)
{
	const auto body = m_world.CreateBoxRigidBody(1.f, glm::vec3(0.5f), glm::vec3(1.f, 2.f, 3.f));

	const glm::vec3 position = PositionOf(body);
	EXPECT_NEAR(position.x, 1.f, 1e-5f);
	EXPECT_NEAR(position.y, 2.f, 1e-5f);
	EXPECT_NEAR(position.z, 3.f, 1e-5f);
}

TEST_F(PhysicsTest, null_handle_transform_queries_fail)
{
	glm::vec3 position;
	glm::quat orientation;

	EXPECT_FALSE(m_world.GetRigidBodyTransform(0, position, orientation));
	EXPECT_FALSE(m_world.SetRigidBodyTransform(0, position, orientation));
	EXPECT_FALSE(m_world.IsStaticBody(0));
}

TEST_F(PhysicsTest, mass_zero_body_is_static)
{
	EXPECT_TRUE(m_world.IsStaticBody(m_world.CreateBoxRigidBody(0.f, glm::vec3(1.f), glm::vec3(0.f))));
	EXPECT_FALSE(m_world.IsStaticBody(m_world.CreateBoxRigidBody(1.f, glm::vec3(1.f), glm::vec3(0.f))));
}

TEST_F(PhysicsTest, dynamic_body_falls_under_gravity)
{
	const auto body = m_world.CreateSphereRigidBody(1.f, 0.5f, glm::vec3(0.f, 10.f, 0.f));

	Simulate(1.f);

	EXPECT_LT(PositionOf(body).y, 9.f);
}

TEST_F(PhysicsTest, static_body_does_not_move)
{
	const auto body = m_world.CreateBoxRigidBody(0.f, glm::vec3(1.f), glm::vec3(0.f, 10.f, 0.f));

	Simulate(1.f);

	EXPECT_NEAR(PositionOf(body).y, 10.f, 1e-5f);
}

TEST_F(PhysicsTest, zero_gravity_keeps_body_in_place)
{
	m_world.SetGravity(glm::vec3(0.f));
	const auto body = m_world.CreateSphereRigidBody(1.f, 0.5f, glm::vec3(0.f, 10.f, 0.f));

	Simulate(1.f);

	EXPECT_NEAR(PositionOf(body).y, 10.f, 1e-3f);
}

TEST_F(PhysicsTest, set_transform_moves_body)
{
	const auto body = m_world.CreateBoxRigidBody(0.f, glm::vec3(1.f), glm::vec3(0.f));

	EXPECT_TRUE(m_world.SetRigidBodyTransform(body, glm::vec3(4.f, 5.f, 6.f), glm::quat(1.f, 0.f, 0.f, 0.f)));

	const glm::vec3 position = PositionOf(body);
	EXPECT_NEAR(position.x, 4.f, 1e-5f);
	EXPECT_NEAR(position.y, 5.f, 1e-5f);
	EXPECT_NEAR(position.z, 6.f, 1e-5f);
}

TEST_F(PhysicsTest, collision_callback_is_called_when_bodies_touch)
{
	const auto ground = m_world.CreateBoxRigidBody(0.f, glm::vec3(10.f, 0.5f, 10.f), glm::vec3(0.f));
	const auto ball = m_world.CreateSphereRigidBody(1.f, 0.5f, glm::vec3(0.f, 3.f, 0.f));

	bool collided = false;
	m_world.RegisterCollisionCallback([&](RigidBodyHandle a, RigidBodyHandle b)
	{
		const bool is_expected_pair = (a == ground && b == ball) || (a == ball && b == ground);
		collided = collided || is_expected_pair;
	});

	Simulate(2.f);

	EXPECT_TRUE(collided);
}
