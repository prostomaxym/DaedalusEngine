#include <Renderer/Objects/DirectionalLightSource.h>

#include <gtest/gtest.h>

#include <glm/gtc/matrix_transform.hpp>

using namespace Daedalus;

namespace
{
	constexpr float k_eps = 1e-4f;

	DirectionalLightSource MakeLight(const glm::vec3& direction)
	{
		LightProps props;
		props.direction = direction;
		return DirectionalLightSource(props);
	}

	glm::vec3 ToNdc(const glm::mat4& m, const glm::vec3& p)
	{
		const glm::vec4 r = m * glm::vec4(p, 1.f);
		return glm::vec3(r) / r.w;
	}
}

TEST(DirectionalLightSourceTest, sphere_projection_maps_center_to_ndc_origin_in_xy)
{
	const auto light = MakeLight({ 0.f, 0.f, 1.f });
	const auto m = light.CalculateProjViewForSphere(BoundingSphere({ 0.f, 0.f, 0.f }, 2.f));

	const auto ndc = ToNdc(m, { 0.f, 0.f, 0.f });
	EXPECT_NEAR(ndc.x, 0.f, k_eps);
	EXPECT_NEAR(ndc.y, 0.f, k_eps);
}

TEST(DirectionalLightSourceTest, sphere_projection_maps_radius_to_ndc_edge)
{
	const auto light = MakeLight({ 0.f, 0.f, 1.f });
	const auto m = light.CalculateProjViewForSphere(BoundingSphere({ 0.f, 0.f, 0.f }, 2.f));

	EXPECT_NEAR(ToNdc(m, { 2.f, 0.f, 0.f }).x, 1.f, k_eps);
	EXPECT_NEAR(ToNdc(m, { -2.f, 0.f, 0.f }).x, -1.f, k_eps);
	EXPECT_NEAR(ToNdc(m, { 0.f, 2.f, 0.f }).y, 1.f, k_eps);
}

TEST(DirectionalLightSourceTest, aabb_projection_maps_bounds_to_ndc_edges)
{
	const auto light = MakeLight({ 0.f, 0.f, 1.f });
	const auto m = light.CalculateProjViewForAABB(AABB({ -1.f, -1.f, -1.f }, { 1.f, 1.f, 1.f }));

	const auto center = ToNdc(m, { 0.f, 0.f, 0.f });
	EXPECT_NEAR(center.x, 0.f, k_eps);
	EXPECT_NEAR(center.y, 0.f, k_eps);

	const auto corner = ToNdc(m, { 1.f, 1.f, 0.f });
	EXPECT_NEAR(corner.x, 1.f, k_eps);
	EXPECT_NEAR(corner.y, 1.f, k_eps);
}

TEST(DirectionalLightSourceTest, frustum_projection_contains_frustum_center)
{
	const auto light = MakeLight(glm::normalize(glm::vec3(1.f, 1.f, 1.f)));
	const glm::mat4 proj = glm::perspective(glm::radians(60.f), 1.f, 1.f, 10.f);
	const glm::mat4 view(1.f);

	const auto m = light.CalculateProjViewForFrustum(proj, view);

	const auto ndc = ToNdc(m, { 0.f, 0.f, -5.f });
	EXPECT_LE(std::abs(ndc.x), 1.f);
	EXPECT_LE(std::abs(ndc.y), 1.f);
	EXPECT_LE(std::abs(ndc.z), 1.f);
}

TEST(DirectionalLightSourceTest, frustum_projection_contains_all_frustum_corners)
{
	const auto light = MakeLight(glm::normalize(glm::vec3(1.f, 2.f, 1.f)));
	const glm::mat4 proj = glm::perspective(glm::radians(60.f), 1.f, 1.f, 10.f);
	const glm::mat4 view(1.f);
	const auto m = light.CalculateProjViewForFrustum(proj, view);
	const glm::mat4 inv = glm::inverse(proj * view);

	for (int x = 0; x < 2; ++x)
	{
		for (int y = 0; y < 2; ++y)
		{
			for (int z = 0; z < 2; ++z)
			{
				const glm::vec4 w = inv * glm::vec4(2.f * x - 1.f, 2.f * y - 1.f, 2.f * z - 1.f, 1.f);
				const auto ndc = ToNdc(m, glm::vec3(w) / w.w);
				EXPECT_LE(std::abs(ndc.x), 1.f + k_eps);
				EXPECT_LE(std::abs(ndc.y), 1.f + k_eps);
				EXPECT_LE(std::abs(ndc.z), 1.f + k_eps);
			}
		}
	}
}
