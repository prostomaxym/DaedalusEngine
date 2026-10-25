#include <Macros.h>
#include <Physics/Physics.h>

#include <gtest/gtest.h>

using namespace Daedalus;

class PhysicsTest : public ::testing::Test
{
protected:
	void SetUp() override { Physics::Init(); }
	void TearDown() override { Physics::Shutdown(); }

	static glm::vec3 PositionOf(RigidBodyHandle body)
	{
		glm::vec3 position;
		glm::quat orientation;
		EXPECT_TRUE(Physics::GetRigidBodyTransform(body, position, orientation));
		return position;
	}

	static void Simulate(float seconds)
	{
		constexpr float step = 1.f / 60.f;
		for (float elapsed = 0.f; elapsed < seconds; elapsed += step)
		{
			Physics::StepSimulation(step);
		}
	}
};

TEST(PhysicsWithoutInitTest, creating_bodies_before_init_returns_null_handle)
{
	Physics::Shutdown();

	EXPECT_EQ(Physics::CreateBoxRigidBody(1.f, glm::vec3(1.f), glm::vec3(0.f)), 0u);
	EXPECT_EQ(Physics::CreateSphereRigidBody(1.f, 1.f, glm::vec3(0.f)), 0u);
}

TEST_F(PhysicsTest, created_bodies_have_non_null_handles)
{
	EXPECT_NE(Physics::CreateBoxRigidBody(1.f, glm::vec3(0.5f), glm::vec3(0.f)), 0u);
	EXPECT_NE(Physics::CreateSphereRigidBody(1.f, 0.5f, glm::vec3(0.f)), 0u);
}

TEST_F(PhysicsTest, created_body_reports_initial_position)
{
	const auto body = Physics::CreateBoxRigidBody(1.f, glm::vec3(0.5f), glm::vec3(1.f, 2.f, 3.f));

	const glm::vec3 position = PositionOf(body);
	EXPECT_NEAR(position.x, 1.f, 1e-5f);
	EXPECT_NEAR(position.y, 2.f, 1e-5f);
	EXPECT_NEAR(position.z, 3.f, 1e-5f);
}

TEST_F(PhysicsTest, null_handle_transform_queries_fail)
{
	glm::vec3 position;
	glm::quat orientation;

	EXPECT_FALSE(Physics::GetRigidBodyTransform(0, position, orientation));
	EXPECT_FALSE(Physics::SetRigidBodyTransform(0, position, orientation));
	EXPECT_FALSE(Physics::IsStaticBody(0));
}

TEST_F(PhysicsTest, mass_zero_body_is_static)
{
	EXPECT_TRUE(Physics::IsStaticBody(Physics::CreateBoxRigidBody(0.f, glm::vec3(1.f), glm::vec3(0.f))));
	EXPECT_FALSE(Physics::IsStaticBody(Physics::CreateBoxRigidBody(1.f, glm::vec3(1.f), glm::vec3(0.f))));
}

TEST_F(PhysicsTest, dynamic_body_falls_under_gravity)
{
	const auto body = Physics::CreateSphereRigidBody(1.f, 0.5f, glm::vec3(0.f, 10.f, 0.f));

	Simulate(1.f);

	EXPECT_LT(PositionOf(body).y, 9.f);
}

TEST_F(PhysicsTest, static_body_does_not_move)
{
	const auto body = Physics::CreateBoxRigidBody(0.f, glm::vec3(1.f), glm::vec3(0.f, 10.f, 0.f));

	Simulate(1.f);

	EXPECT_NEAR(PositionOf(body).y, 10.f, 1e-5f);
}

TEST_F(PhysicsTest, zero_gravity_keeps_body_in_place)
{
	Physics::SetGravity(glm::vec3(0.f));
	const auto body = Physics::CreateSphereRigidBody(1.f, 0.5f, glm::vec3(0.f, 10.f, 0.f));

	Simulate(1.f);

	EXPECT_NEAR(PositionOf(body).y, 10.f, 1e-3f);
}

TEST_F(PhysicsTest, set_transform_moves_body)
{
	const auto body = Physics::CreateBoxRigidBody(0.f, glm::vec3(1.f), glm::vec3(0.f));

	EXPECT_TRUE(Physics::SetRigidBodyTransform(body, glm::vec3(4.f, 5.f, 6.f), glm::quat(1.f, 0.f, 0.f, 0.f)));

	const glm::vec3 position = PositionOf(body);
	EXPECT_NEAR(position.x, 4.f, 1e-5f);
	EXPECT_NEAR(position.y, 5.f, 1e-5f);
	EXPECT_NEAR(position.z, 6.f, 1e-5f);
}

TEST_F(PhysicsTest, collision_callback_is_called_when_bodies_touch)
{
	const auto ground = Physics::CreateBoxRigidBody(0.f, glm::vec3(10.f, 0.5f, 10.f), glm::vec3(0.f));
	const auto ball = Physics::CreateSphereRigidBody(1.f, 0.5f, glm::vec3(0.f, 3.f, 0.f));

	bool collided = false;
	Physics::RegisterCollisionCallback([&](RigidBodyHandle a, RigidBodyHandle b)
	{
		const bool is_expected_pair = (a == ground && b == ball) || (a == ball && b == ground);
		collided = collided || is_expected_pair;
	});

	Simulate(2.f);

	EXPECT_TRUE(collided);
}
