#include <Renderer/Objects/Geometry.h>

#include <gtest/gtest.h>

#include <glm/gtc/matrix_transform.hpp>

using namespace Daedalus;

namespace
{
	constexpr float k_eps = 1e-4f;

	void ExpectVecNear(const glm::vec3& a, const glm::vec3& b)
	{
		EXPECT_NEAR(a.x, b.x, k_eps);
		EXPECT_NEAR(a.y, b.y, k_eps);
		EXPECT_NEAR(a.z, b.z, k_eps);
	}
}

TEST(BoundingSphereTest, transformed_sphere_applies_translation)
{
	const BoundingSphere sphere(glm::vec3(1.f, 0.f, 0.f), 2.f);
	const auto result = sphere.GetTransformedSphere(glm::translate(glm::mat4(1.f), glm::vec3(0.f, 5.f, 0.f)));

	ExpectVecNear(result.position, glm::vec3(1.f, 5.f, 0.f));
	EXPECT_NEAR(result.radius, 2.f, k_eps);
}

TEST(BoundingSphereTest, transformed_sphere_scales_radius_by_max_axis)
{
	const BoundingSphere sphere(glm::vec3(0.f, 0.f, 10.f), 1.f);
	const auto result = sphere.GetTransformedSphere(glm::scale(glm::mat4(1.f), glm::vec3(1.f, 3.f, 2.f)));

	EXPECT_NEAR(result.radius, 3.f, k_eps);
}

TEST(BoundingSphereTest, common_sphere_of_empty_list_is_default)
{
	const auto result = BoundingSphere::CalculateCommonBoundingSphere({});

	ExpectVecNear(result.position, glm::vec3(0.f));
	EXPECT_FLOAT_EQ(result.radius, 0.f);
}

TEST(BoundingSphereTest, common_sphere_encloses_all_spheres)
{
	const std::vector<BoundingSphere> spheres = {
		BoundingSphere(glm::vec3(-2.f, 0.f, 0.f), 1.f),
		BoundingSphere(glm::vec3(2.f, 0.f, 0.f), 1.f)
	};

	const auto result = BoundingSphere::CalculateCommonBoundingSphere(spheres);

	ExpectVecNear(result.position, glm::vec3(0.f));
	EXPECT_NEAR(result.radius, 3.f, k_eps);
}

TEST(AABBTest, constructor_computes_center_and_extents)
{
	const AABB box(glm::vec3(-1.f, -2.f, -3.f), glm::vec3(3.f, 2.f, 1.f));

	ExpectVecNear(box.center, glm::vec3(1.f, 0.f, -1.f));
	ExpectVecNear(box.extents, glm::vec3(2.f, 2.f, 2.f));
	ExpectVecNear(box.GetMin(), glm::vec3(-1.f, -2.f, -3.f));
	ExpectVecNear(box.GetMax(), glm::vec3(3.f, 2.f, 1.f));
}

TEST(AABBTest, common_aabb_of_empty_list_is_zero)
{
	const auto result = AABB::CalculateCommonBoundingAABB({});

	ExpectVecNear(result.GetMin(), glm::vec3(0.f));
	ExpectVecNear(result.GetMax(), glm::vec3(0.f));
}

TEST(AABBTest, common_aabb_covers_all_boxes)
{
	const std::vector<AABB> boxes = {
		AABB(glm::vec3(0.f), glm::vec3(1.f)),
		AABB(glm::vec3(-2.f, 0.5f, 0.f), glm::vec3(0.f, 4.f, 3.f))
	};

	const auto result = AABB::CalculateCommonBoundingAABB(boxes);

	ExpectVecNear(result.GetMin(), glm::vec3(-2.f, 0.f, 0.f));
	ExpectVecNear(result.GetMax(), glm::vec3(1.f, 4.f, 3.f));
}

TEST(FrustumTest, sphere_inside_frustum_is_visible)
{
	const glm::mat4 pv = glm::perspective(glm::radians(90.f), 1.f, 0.1f, 100.f);
	const auto frustum = Frustum::CalculateFrustum(pv);

	EXPECT_TRUE(frustum.SphereInFrustum(BoundingSphere(glm::vec3(0.f, 0.f, -10.f), 1.f), glm::mat4(1.f)));
}

TEST(FrustumTest, sphere_behind_camera_is_culled)
{
	const glm::mat4 pv = glm::perspective(glm::radians(90.f), 1.f, 0.1f, 100.f);
	const auto frustum = Frustum::CalculateFrustum(pv);

	EXPECT_FALSE(frustum.SphereInFrustum(BoundingSphere(glm::vec3(0.f, 0.f, 10.f), 1.f), glm::mat4(1.f)));
}

TEST(FrustumTest, sphere_beyond_far_plane_is_culled)
{
	const glm::mat4 pv = glm::perspective(glm::radians(90.f), 1.f, 0.1f, 100.f);
	const auto frustum = Frustum::CalculateFrustum(pv);

	EXPECT_FALSE(frustum.SphereInFrustum(BoundingSphere(glm::vec3(0.f, 0.f, -200.f), 1.f), glm::mat4(1.f)));
}

TEST(FrustumTest, sphere_far_to_the_side_is_culled)
{
	const glm::mat4 pv = glm::perspective(glm::radians(90.f), 1.f, 0.1f, 100.f);
	const auto frustum = Frustum::CalculateFrustum(pv);

	EXPECT_FALSE(frustum.SphereInFrustum(BoundingSphere(glm::vec3(50.f, 0.f, -10.f), 1.f), glm::mat4(1.f)));
}

TEST(FrustumTest, object_transform_moves_sphere_into_view)
{
	const glm::mat4 pv = glm::perspective(glm::radians(90.f), 1.f, 0.1f, 100.f);
	const auto frustum = Frustum::CalculateFrustum(pv);
	const BoundingSphere sphere(glm::vec3(0.f, 0.f, 10.f), 1.f);

	EXPECT_FALSE(frustum.SphereInFrustum(sphere, glm::mat4(1.f)));
	EXPECT_TRUE(frustum.SphereInFrustum(sphere, glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -20.f))));
}
