#include <gtest/gtest.h>
#include <Debug/WorldAxisProjection.h>
#include <Renderer/Objects/PerspectiveCamera.h>
#include <Renderer/Objects/OrthographicCamera.h>
#include <algorithm>
#include <cmath>
#include <limits>

using namespace Daedalus;

namespace
{
	constexpr float Epsilon = 0.0001f;

	void ExpectOffsets(const std::array<glm::vec2, 3>& actual, const std::array<glm::vec2, 3>& expected)
	{
		for (size_t axis = 0; axis < actual.size(); ++axis)
		{
			EXPECT_NEAR(actual[axis].x, expected[axis].x, Epsilon) << axis;
			EXPECT_NEAR(actual[axis].y, expected[axis].y, Epsilon) << axis;
		}
	}
}

TEST(WorldAxisProjectionTest, ForwardCameraShowsWorldXRightAndWorldYUp)
{
	PerspectiveCamera camera(CameraProjectionProps(80.f, 16.f / 9.f, 0.1f, 100.f),
		CameraPositionProps(glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f), -90.f, 0.f));
	std::array<glm::vec2, 3> offsets;
	ASSERT_TRUE(WorldAxisProjection::Calculate(camera, offsets));
	EXPECT_NEAR(offsets[0].x, 1.f, Epsilon);
	EXPECT_NEAR(offsets[0].y, 0.f, Epsilon);
	EXPECT_NEAR(offsets[1].x, 0.f, Epsilon);
	EXPECT_NEAR(offsets[1].y, -1.f, Epsilon);
	// Perspective retains the smoke harness's lower-origin parallax for the depth axis.
	EXPECT_NEAR(offsets[2].x, 0.f, Epsilon);
	EXPECT_GT(offsets[2].y, 0.f);
}

TEST(WorldAxisProjectionTest, LookingAlongWorldXDoesNotRelabelCameraRightAsWorldX)
{
	PerspectiveCamera camera(CameraProjectionProps(80.f, 1.f, 0.1f, 100.f),
		CameraPositionProps(glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f), 0.f, 0.f));
	std::array<glm::vec2, 3> offsets;
	ASSERT_TRUE(WorldAxisProjection::Calculate(camera, offsets));
	EXPECT_NEAR(offsets[0].x, 0.f, Epsilon);
	EXPECT_NEAR(offsets[1].y, -1.f, Epsilon);
	EXPECT_NEAR(offsets[2].x, 1.f, Epsilon);
	EXPECT_NEAR(offsets[2].y, 0.f, Epsilon);
}

TEST(WorldAxisProjectionTest, RuntimeThirdPersonOrientationMatchesSmokeSegmentProjection)
{
	const CameraPositionProps position(glm::vec3(0.f, 0.f, -1.87673712f), glm::vec3(0.f, 1.f, 0.f), -113.058838f, -85.0253525f);
	PerspectiveCamera camera(CameraProjectionProps(80.f, 16.f / 9.f, 0.1f, 100.f), position);
	camera.SetPersonParams(7.f, 4.f);
	std::array<glm::vec2, 3> offsets;
	ASSERT_TRUE(WorldAxisProjection::Calculate(camera, offsets));

	// Independently project the harness's world-space submissions at these clip settings.
	const float distance = 5.1f;
	const float size = distance * std::tan(glm::radians(40.f)) * 0.12f;
	const auto origin = camera.GetPosition() + camera.GetDirection() * distance - camera.GetUp() * (size * 1.6f);
	const auto matrix = camera.GetProjectionViewMatrix();
	const auto project = [&](glm::vec3 point)
	{
		const auto clip = matrix * glm::vec4(point, 1.f);
		return glm::vec2(clip.x / clip.w * (16.f / 9.f), -clip.y / clip.w);
	};
	const glm::vec3 world_axes[] = { { 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f } };
	std::array<glm::vec2, 3> expected;
	float radius = 0.f;
	for (size_t axis = 0; axis < expected.size(); ++axis)
	{
		expected[axis] = project(origin + world_axes[axis] * size) - project(origin);
		radius = std::max(radius, glm::length(expected[axis]));
	}
	for (auto& offset : expected) offset /= radius;
	ExpectOffsets(offsets, expected);

	camera.SetPosition(glm::vec3(100.f, -30.f, 40.f));
	ASSERT_TRUE(WorldAxisProjection::Calculate(camera, offsets));
	ExpectOffsets(offsets, expected);
}

TEST(WorldAxisProjectionTest, OrthographicProjectionPreservesEndOnAxisAndAspect)
{
	OrthographicCamera camera(-4.f, 4.f, -2.f, 2.f, 0.1f, 100.f,
		CameraPositionProps(glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f), -90.f, 0.f));
	std::array<glm::vec2, 3> offsets;
	ASSERT_TRUE(WorldAxisProjection::Calculate(camera, offsets));
	const std::array<glm::vec2, 3> expected = { glm::vec2(1.f, 0.f), glm::vec2(0.f, -1.f), glm::vec2(0.f) };
	ExpectOffsets(offsets, expected);
}

TEST(WorldAxisProjectionTest, NarrowClipRangeStillProducesFiniteAxes)
{
	PerspectiveCamera camera(CameraProjectionProps(80.f, 1.f, 1.f, 1.01f),
		CameraPositionProps(glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f), -90.f, 0.f));
	std::array<glm::vec2, 3> offsets;
	ASSERT_TRUE(WorldAxisProjection::Calculate(camera, offsets));
	for (const auto& offset : offsets)
	{
		EXPECT_TRUE(std::isfinite(offset.x));
		EXPECT_TRUE(std::isfinite(offset.y));
	}
}

TEST(WorldAxisProjectionTest, InvalidClipRangeClearsOutput)
{
	PerspectiveCamera camera(CameraProjectionProps(80.f, 1.f, 2.f, 1.f));
	std::array<glm::vec2, 3> offsets = { glm::vec2(1.f), glm::vec2(1.f), glm::vec2(1.f) };
	EXPECT_FALSE(WorldAxisProjection::Calculate(camera, offsets));
	for (const auto& offset : offsets) EXPECT_EQ(offset, glm::vec2(0.f));
}

TEST(WorldAxisProjectionTest, NonfiniteProjectionAndClippedThirdPersonPlacementAreRejected)
{
	PerspectiveCamera invalid_camera(CameraProjectionProps(std::numeric_limits<float>::quiet_NaN(), 1.f, 0.1f, 100.f));
	std::array<glm::vec2, 3> offsets;
	EXPECT_FALSE(WorldAxisProjection::Calculate(invalid_camera, offsets));
	PerspectiveCamera camera(CameraProjectionProps(80.f, 1.f, 0.1f, 100.f));
	camera.SetPersonParams(200.f, 0.f);
	EXPECT_FALSE(WorldAxisProjection::Calculate(camera, offsets));
}
