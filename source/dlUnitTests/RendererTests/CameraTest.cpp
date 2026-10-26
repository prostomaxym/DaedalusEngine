#include <gtest/gtest.h>

#include <Renderer/Objects/PerspectiveCamera.h>
#include <Renderer/Objects/OrthographicCamera.h>

using namespace Daedalus;

namespace
{
	constexpr float k_eps = 1e-4f;
}

TEST(CameraTest, initial_direction_follows_yaw_and_pitch)
{
	PerspectiveCamera camera(CameraProjectionProps(), CameraPositionProps(glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f), -90.f, 0.f));

	EXPECT_NEAR(camera.GetDirection().x, 0.f, k_eps);
	EXPECT_NEAR(camera.GetDirection().z, -1.f, k_eps);
	EXPECT_NEAR(camera.GetRight().x, 1.f, k_eps);
	EXPECT_NEAR(camera.GetUp().y, 1.f, k_eps);
}

TEST(CameraTest, get_pitch_returns_pitch_not_yaw)
{
	PerspectiveCamera camera(CameraProjectionProps(), CameraPositionProps(glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f), 30.f, 10.f));

	EXPECT_FLOAT_EQ(camera.GetYaw(), 30.f);
	EXPECT_FLOAT_EQ(camera.GetPitch(), 10.f);
}

TEST(CameraTest, set_position_updates_position)
{
	PerspectiveCamera camera;
	camera.SetPosition(glm::vec3(1.f, 2.f, 3.f));
	EXPECT_EQ(camera.GetPosition(), glm::vec3(1.f, 2.f, 3.f));
}

TEST(CameraTest, keyboard_forward_moves_along_front)
{
	PerspectiveCamera camera(CameraProjectionProps(), CameraPositionProps(glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f), 0.f, 0.f));
	camera.SetMovementSpeed(2.f);
	camera.ProcessKeyboard(CameraMovement::FORWARD, 0.5f);

	EXPECT_NEAR(camera.GetPosition().x, 1.f, k_eps);
	EXPECT_NEAR(camera.GetPosition().y, 0.f, k_eps);
}

TEST(CameraTest, move_up_and_down_use_world_up)
{
	PerspectiveCamera camera;
	camera.MoveCamera(CameraMovement::UP, 2.f);
	EXPECT_NEAR(camera.GetPosition().y, 2.f, k_eps);
	camera.MoveCamera(CameraMovement::DOWN, 3.f);
	EXPECT_NEAR(camera.GetPosition().y, -1.f, k_eps);
}

TEST(CameraTest, mouse_movement_constrains_pitch)
{
	PerspectiveCamera camera;
	camera.SetSensivitity(1.f);
	camera.ProcessMouseMovement(0.f, 500.f);
	EXPECT_FLOAT_EQ(camera.GetPitch(), 89.f);
	camera.ProcessMouseMovement(0.f, -1000.f);
	EXPECT_FLOAT_EQ(camera.GetPitch(), -89.f);
}

TEST(CameraTest, mouse_movement_without_constraint_allows_large_pitch)
{
	PerspectiveCamera camera;
	camera.SetSensivitity(1.f);
	camera.ProcessMouseMovement(0.f, 120.f, false);
	EXPECT_FLOAT_EQ(camera.GetPitch(), 120.f);
}

TEST(CameraTest, rotate_camera_accumulates_and_set_rotation_overrides)
{
	PerspectiveCamera camera;
	camera.RotateCamera(10.f, 5.f);
	camera.RotateCamera(10.f, 5.f);
	EXPECT_FLOAT_EQ(camera.GetYaw(), 20.f);
	EXPECT_FLOAT_EQ(camera.GetPitch(), 10.f);

	camera.SetRotation(1.f, 2.f);
	EXPECT_FLOAT_EQ(camera.GetYaw(), 1.f);
	EXPECT_FLOAT_EQ(camera.GetPitch(), 2.f);
}

TEST(CameraTest, perspective_projection_matches_glm)
{
	PerspectiveCamera camera(CameraProjectionProps(60.f, 2.f, 0.5f, 50.f));

	const glm::mat4 actual = camera.GetProjectionMatrix();
	const glm::mat4 expected = glm::perspective(glm::radians(60.f), 2.f, 0.5f, 50.f);
	// Constant-folded and runtime trigonometry need not produce bit-identical floats.
	for (glm::length_t column = 0; column < 4; ++column)
	{
		for (glm::length_t row = 0; row < 4; ++row)
		{
			EXPECT_FLOAT_EQ(actual[column][row], expected[column][row])
				<< "column=" << column << ", row=" << row;
		}
	}
	EXPECT_FLOAT_EQ(camera.GetNearPlane(), 0.5f);
	EXPECT_FLOAT_EQ(camera.GetFarPlane(), 50.f);
}

TEST(CameraTest, perspective_projection_maps_frustum_boundaries_to_ndc)
{
	const PerspectiveCamera camera(CameraProjectionProps(90.f, 2.f, 0.5f, 50.f));
	const glm::mat4 projection = camera.GetProjectionMatrix();
	struct ProjectionCase
	{
		glm::vec4 view_position;
		glm::vec3 expected_ndc;
	};
	// At a 90-degree vertical FOV, the near-plane half-height is 0.5 and half-width is 1.
	const ProjectionCase cases[] =
	{
		{ glm::vec4(0.f, 0.f, -0.5f, 1.f), glm::vec3(0.f, 0.f, -1.f) },
		{ glm::vec4(0.f, 0.f, -50.f, 1.f), glm::vec3(0.f, 0.f, 1.f) },
		{ glm::vec4(1.f, 0.5f, -0.5f, 1.f), glm::vec3(1.f, 1.f, -1.f) },
		{ glm::vec4(-1.f, -0.5f, -0.5f, 1.f), glm::vec3(-1.f, -1.f, -1.f) }
	};

	for (const auto& test_case : cases)
	{
		const glm::vec4 clip_position = projection * test_case.view_position;
		ASSERT_GT(clip_position.w, 0.f);
		const glm::vec3 ndc = glm::vec3(clip_position) / clip_position.w;
		for (glm::length_t component = 0; component < 3; ++component)
		{
			EXPECT_NEAR(ndc[component], test_case.expected_ndc[component], k_eps)
				<< "view_z=" << test_case.view_position.z << ", component=" << component;
		}
	}
}

TEST(CameraTest, view_projection_is_projection_times_view)
{
	PerspectiveCamera camera;
	camera.SetPosition(glm::vec3(1.f, 2.f, 3.f));
	EXPECT_EQ(camera.GetProjectionViewMatrix(), camera.GetProjectionMatrix() * camera.GetViewMatrix());
}

TEST(CameraTest, orthographic_projection_matches_glm)
{
	OrthographicCamera camera(-2.f, 2.f, -1.f, 1.f, 0.1f, 20.f);

	EXPECT_EQ(camera.GetProjectionMatrix(), glm::ortho(-2.f, 2.f, -1.f, 1.f, 0.1f, 20.f));
	EXPECT_FLOAT_EQ(camera.GetNearPlane(), 0.1f);
	EXPECT_FLOAT_EQ(camera.GetFarPlane(), 20.f);
}

TEST(CameraTest, orthographic_set_projection_updates_planes)
{
	OrthographicCamera camera(-1.f, 1.f, -1.f, 1.f, 0.f, 1.f);
	camera.SetProjection(-4.f, 4.f, -3.f, 3.f, 1.f, 5.f);

	EXPECT_EQ(camera.GetProjectionMatrix(), glm::ortho(-4.f, 4.f, -3.f, 3.f, 1.f, 5.f));
}
