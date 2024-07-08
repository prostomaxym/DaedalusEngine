#include "dlpch.h"

#include "SpotLightSource.h"

using namespace Daedalus;

namespace
{
	// Stupid ChatGPT gave me this shitty code. No time to clean, sry
	void ExtractPerspectiveParams(const glm::mat4& projMatrix, float& fov, float& aspect, float& z_near, float& z_far)
	{
		aspect = projMatrix[1][1] / projMatrix[0][0];
		fov = 2.0f * atan(1.0f / projMatrix[1][1]);
		z_near = (projMatrix[3][2]) / (projMatrix[2][2] - 1.0f);
		z_far = (projMatrix[3][2]) / (projMatrix[2][2] + 1.0f);
	}
}

SpotLightSource::SpotLightSource(glm::vec3 light_pos
	, glm::vec3 ambient_color
	, glm::vec3 diffuse_color
	, glm::vec3 specular_color
	, bool cast_shadows
	, float light_power
	, float max_distance
	, glm::vec3 direction
	, float cutoff_angle
	, float outer_cutoff) :
    LightSource(LightSourceType::Spot, light_pos, ambient_color, diffuse_color, specular_color, cast_shadows, light_power, max_distance, direction, std::cos(glm::radians(cutoff_angle)), std::cos(glm::radians(outer_cutoff)))
{}

void SpotLightSource::UpdateSSBODefault()
{
	m_params.proj_view = CalculateLightMatrixDefault();
}

glm::mat4 SpotLightSource::CalculateLightMatrixForFrustum(const glm::mat4& camera_proj, const glm::mat4&) const
{
	const auto light_view = glm::lookAt(
		m_params.position,
		m_params.position + m_params.direction,
		glm::vec3(0.0f, 1.0f, 0.0f)
	);

	float fov = 0.f, aspect = 0.f, znear = 0.f, zfar = 0.f;
	ExtractPerspectiveParams(camera_proj, fov, aspect, znear, zfar);

	const glm::mat4 light_projection = glm::perspective(fov, 1.f, znear, m_max_distance);

	return light_projection * light_view;
}

glm::mat4 SpotLightSource::CalculateLightMatrixDefault() const
{
	const auto light_view = glm::lookAt(
		m_params.position,
		m_params.position + m_params.direction,
		glm::vec3(0.0f, 1.0f, 0.0f)
	);

	const glm::mat4 light_projection = glm::perspective(90.f, 1.f, 0.f, m_params.GetMaxDistance());

	// Combine view and projection matrices
	return light_projection * light_view;
}