#include <ECS/Components.h>

#include <gtest/gtest.h>

#include <glm/gtc/constants.hpp>

using Daedalus::TransformComponent;

TEST(TransformComponentTest, default_is_identity)
{
	const TransformComponent transform;
	const glm::mat4 matrix = transform.GetTransform();

	for (int column = 0; column < 4; ++column)
	{
		for (int row = 0; row < 4; ++row)
		{
			EXPECT_NEAR(matrix[column][row], column == row ? 1.f : 0.f, 1e-6f);
		}
	}
}

TEST(TransformComponentTest, translation_ends_up_in_last_column)
{
	const TransformComponent transform(glm::vec3(1.f, 2.f, 3.f));
	const glm::mat4 matrix = transform.GetTransform();

	EXPECT_FLOAT_EQ(matrix[3][0], 1.f);
	EXPECT_FLOAT_EQ(matrix[3][1], 2.f);
	EXPECT_FLOAT_EQ(matrix[3][2], 3.f);
}

TEST(TransformComponentTest, scale_affects_basis_vectors)
{
	TransformComponent transform;
	transform.scale = glm::vec3(2.f, 3.f, 4.f);
	const glm::mat4 matrix = transform.GetTransform();

	EXPECT_FLOAT_EQ(matrix[0][0], 2.f);
	EXPECT_FLOAT_EQ(matrix[1][1], 3.f);
	EXPECT_FLOAT_EQ(matrix[2][2], 4.f);
}

TEST(TransformComponentTest, rotation_is_stored_in_degrees)
{
	TransformComponent transform;
	transform.rotation = glm::vec3(0.f, 90.f, 0.f);

	const glm::vec4 rotated = transform.GetTransform() * glm::vec4(1.f, 0.f, 0.f, 1.f);

	EXPECT_NEAR(rotated.x, 0.f, 1e-5f);
	EXPECT_NEAR(rotated.y, 0.f, 1e-5f);
	EXPECT_NEAR(rotated.z, -1.f, 1e-5f);
}

TEST(TransformComponentTest, orientation_quaternion_round_trip)
{
	TransformComponent transform;
	transform.rotation = glm::vec3(10.f, 20.f, 30.f);

	TransformComponent other;
	other.SetOrientationQuat(transform.GetOrientationQuat());

	EXPECT_NEAR(other.rotation.x, 10.f, 1e-3f);
	EXPECT_NEAR(other.rotation.y, 20.f, 1e-3f);
	EXPECT_NEAR(other.rotation.z, 30.f, 1e-3f);
}
