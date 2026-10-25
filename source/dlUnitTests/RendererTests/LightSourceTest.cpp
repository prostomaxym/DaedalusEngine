#include <Renderer/Objects/DirectionalLightSource.h>
#include <Renderer/Objects/PointLightSource.h>
#include <Renderer/Objects/SpotLightSource.h>

#include <gtest/gtest.h>

#include <glm/gtc/matrix_transform.hpp>

using namespace Daedalus;

namespace
{
	constexpr float k_eps = 1e-4f;

	LightProps MakeProps()
	{
		LightProps props;
		props.light_pos = glm::vec3(1.f, 2.f, 3.f);
		props.direction = glm::vec3(0.f, -1.f, 0.f);
		props.ambient_color = glm::vec3(0.1f);
		props.diffuse_color = glm::vec3(0.5f);
		props.specular_color = glm::vec3(1.f);
		props.light_power = 2.f;
		props.max_distance = 100.f;
		props.theta_angle = 60.f;
		props.outer_cutoff = 90.f;
		props.cast_shadows = true;
		return props;
	}
}

TEST(LightSourceTest, props_are_copied_into_ssbo)
{
	const PointLightSource light(MakeProps());
	const LightSSBO& ssbo = light.GetShaderSSBO();

	EXPECT_EQ(ssbo.position, glm::vec3(1.f, 2.f, 3.f));
	EXPECT_EQ(ssbo.direction, glm::vec3(0.f, -1.f, 0.f));
	EXPECT_EQ(ssbo.ambient_color, glm::vec3(0.1f));
	EXPECT_EQ(ssbo.diffuse_color, glm::vec3(0.5f));
	EXPECT_EQ(ssbo.specular_color, glm::vec3(1.f));
	EXPECT_FLOAT_EQ(ssbo.power, 2.f);
	EXPECT_EQ(ssbo.shadow_map_index, -1);
}

TEST(LightSourceTest, cutoff_angles_are_stored_as_cosines)
{
	const SpotLightSource light(MakeProps());

	EXPECT_NEAR(light.GetShaderSSBO().cutoff_angle, std::cos(glm::radians(60.f)), k_eps);
	EXPECT_NEAR(light.GetShaderSSBO().outer_cutoff_angle, std::cos(glm::radians(90.f)), k_eps);
}

TEST(LightSourceTest, light_types_are_set_by_subclass)
{
	EXPECT_EQ(DirectionalLightSource(MakeProps()).GetShaderSSBO().type, LightSourceType::Directional);
	EXPECT_EQ(PointLightSource(MakeProps()).GetShaderSSBO().type, LightSourceType::Point);
	EXPECT_EQ(SpotLightSource(MakeProps()).GetShaderSSBO().type, LightSourceType::Spot);
}

TEST(LightSourceTest, cast_shadow_flag_follows_props)
{
	LightProps props = MakeProps();
	EXPECT_TRUE(PointLightSource(props).CastShadow());

	props.cast_shadows = false;
	EXPECT_FALSE(PointLightSource(props).CastShadow());
}

TEST(LightSourceTest, attenuation_for_100_units_matches_table)
{
	const PointLightSource light(MakeProps());
	const LightSSBO& ssbo = light.GetShaderSSBO();

	EXPECT_FLOAT_EQ(ssbo.constant, 1.f);
	EXPECT_FLOAT_EQ(ssbo.linear, 0.045f);
	EXPECT_FLOAT_EQ(ssbo.quadratic, 0.0075f);
}

TEST(LightSourceTest, attenuation_clamps_to_table_limits)
{
	LightProps props = MakeProps();

	props.max_distance = 1.f;
	EXPECT_FLOAT_EQ(PointLightSource(props).GetShaderSSBO().linear, 0.7f);

	props.max_distance = 100000.f;
	EXPECT_FLOAT_EQ(PointLightSource(props).GetShaderSSBO().linear, 0.0014f);
}

TEST(LightSourceTest, set_max_distance_updates_attenuation)
{
	PointLightSource light(MakeProps());
	light.SetMaxDistance(50.f);

	EXPECT_FLOAT_EQ(light.GetShaderSSBO().linear, 0.09f);
	EXPECT_FLOAT_EQ(light.GetShaderSSBO().quadratic, 0.032f);
}

TEST(LightSourceTest, setters_update_ssbo)
{
	PointLightSource light(MakeProps());
	light.SetPosition(glm::vec3(9.f, 8.f, 7.f));
	light.SetDirection(glm::vec3(1.f, 0.f, 0.f));
	light.SetPower(5.f);
	light.SetShadowMapIndex(3);

	const LightSSBO& ssbo = light.GetShaderSSBO();
	EXPECT_EQ(ssbo.position, glm::vec3(9.f, 8.f, 7.f));
	EXPECT_EQ(ssbo.direction, glm::vec3(1.f, 0.f, 0.f));
	EXPECT_FLOAT_EQ(ssbo.power, 5.f);
	EXPECT_EQ(ssbo.shadow_map_index, 3);
}

TEST(LightSourceTest, ssbo_layout_is_16_byte_aligned)
{
	EXPECT_EQ(sizeof(LightSSBO), 112u);
	EXPECT_EQ(sizeof(LightSSBO) % 16u, 0u);
}

TEST(LightSourceTest, point_light_has_six_shadow_faces)
{
	const PointLightSource light(MakeProps());
	EXPECT_EQ(light.GetShadowNumberOfCascades(), 6);
}

TEST(LightSourceTest, point_light_faces_use_90_degree_fov)
{
	LightProps props = MakeProps();
	props.light_pos = glm::vec3(0.f);
	const PointLightSource light(props);
	const glm::mat4 camera_proj = glm::perspective(glm::radians(70.f), 1.f, 0.1f, 100.f);

	const auto faces = light.CalculateCascadesProjView(camera_proj, glm::mat4(1.f));
	ASSERT_EQ(faces.size(), 6u);

	// A point 5 units along +X and 4.5 sideways is inside a 90 degree face (4.5 / 5 < 1).
	const glm::vec4 clip = faces[0] * glm::vec4(5.f, 4.5f, 0.f, 1.f);
	ASSERT_GT(clip.w, 0.f);
	EXPECT_LT(std::abs(clip.x / clip.w), 1.f);
	EXPECT_LT(std::abs(clip.y / clip.w), 1.f);

	// And a point 5.5 sideways is outside.
	const glm::vec4 outside = faces[0] * glm::vec4(5.f, 5.5f, 0.f, 1.f);
	EXPECT_GT(std::abs(outside.y / outside.w), 1.f);
}

TEST(LightSourceTest, spot_light_projection_looks_along_direction)
{
	LightProps props = MakeProps();
	props.light_pos = glm::vec3(0.f);
	props.direction = glm::vec3(0.f, 0.f, -1.f);
	const SpotLightSource light(props);
	const glm::mat4 camera_proj = glm::perspective(glm::radians(70.f), 1.f, 0.1f, 100.f);

	const glm::mat4 pv = light.CalculateProjViewForFrustum(camera_proj, glm::mat4(1.f));
	const glm::vec4 clip = pv * glm::vec4(0.f, 0.f, -10.f, 1.f);

	ASSERT_GT(clip.w, 0.f);
	EXPECT_NEAR(clip.x / clip.w, 0.f, k_eps);
	EXPECT_NEAR(clip.y / clip.w, 0.f, k_eps);
}
