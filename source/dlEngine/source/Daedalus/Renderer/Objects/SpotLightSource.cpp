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
    m_params(2, light_pos, ambient_color, diffuse_color, specular_color, cast_shadows, light_power, max_distance, direction, std::cos(glm::radians(cutoff_angle)), std::cos(glm::radians(outer_cutoff)))
	, m_max_distance(max_distance)
{}

void SpotLightSource::UpdateSSBOForViewFrustum(const glm::mat4& proj, const glm::mat4& view)
{
	m_params.proj_view = CalculateLightMatrixForFrustum(proj, view);
}

void SpotLightSource::UpdateSSBOForSceneSphere(const BoundingSphere& sphere)
{
	m_params.proj_view = CalculateLightMatrixForSphere(sphere);
}

void SpotLightSource::UpdateSSBOForSceneAABB(const AABB& aabb)
{
	m_params.proj_view = CalculateLightMatrixForAABB(aabb);
}

void SpotLightSource::SetMaxDistance(float distance)
{
	m_params.SetMaxDistance(distance);
}

glm::mat4 SpotLightSource::CalculateLightMatrixForFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view) const
{
	const auto inv = glm::inverse(camera_proj * camera_view);

	std::array<glm::vec4, 8> frustum_corners;
	for (unsigned int x = 0, i = 0; x < 2; ++x)
	{
		for (unsigned int y = 0; y < 2; ++y)
		{
			for (unsigned int z = 0; z < 2; ++z)
			{
				const glm::vec4 pt =
					inv * glm::vec4(
						2.0f * x - 1.0f,
						2.0f * y - 1.0f,
						2.0f * z - 1.0f,
						1.0f);
				frustum_corners[i] = (pt / pt.w);
				i++;
			}
		}
	}

	glm::vec3 center = glm::vec3(0, 0, 0);
	for (const auto& v : frustum_corners)
	{
		center += glm::vec3(v);
	}
	center /= frustum_corners.size();

	const auto light_view = glm::lookAt(
		m_params.position,
		m_params.position + m_params.direction,
		glm::vec3(0.0f, 1.0f, 0.0f)
	);

	float fov = 0.f, aspect = 0.f, znear = 0.f, zfar = 0.f;
	ExtractPerspectiveParams(camera_proj, fov, aspect, znear, zfar);

	auto min_dist = std::numeric_limits<float>::max();
	auto max_dist = std::numeric_limits<float>::min();

	for (const auto& corner : frustum_corners)
	{
		const auto dist = glm::distance(glm::vec4(m_params.position, 1.f), corner);

		if (dist < min_dist)
			min_dist = dist;

		if (dist > max_dist)
			max_dist = dist;
	}

	const glm::mat4 light_projection = glm::perspective(fov, 1.f, znear, zfar);

	return light_projection * light_view;
}

glm::mat4 Daedalus::SpotLightSource::CalculateLightMatrixForSphere(const BoundingSphere& sphere) const
{
	// Calculate the light's view matrix
	glm::vec3 center = sphere.position;
	glm::vec3 eye = center + m_params.direction;
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::mat4 light_view = glm::lookAt(eye, center, up);

	// Calculate the light's projection matrix
	float radius = sphere.radius;
	float aspectRatio = 1.0f;  // Assuming a square aspect ratio for simplicity
	float nearPlane = radius;
	float farPlane = 2.0f * radius;
	glm::mat4 light_projection = glm::ortho(-radius, radius, -radius, radius, nearPlane, farPlane);

	// Combine view and projection matrices
	return light_projection * light_view;
}

glm::mat4 Daedalus::SpotLightSource::CalculateLightMatrixForAABB(const AABB& aabb) const
{
	// Calculate the light's view matrix
	glm::vec3 center = aabb.center;
	glm::vec3 eye = center + m_params.direction;
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::mat4 light_view = glm::lookAt(eye, center, up);

	// Calculate the light's projection matrix
	glm::vec3 extents = aabb.extents;
	float maxExtent = glm::length(extents);
	float aspectRatio = 1.0f;  // Assuming a square aspect ratio for simplicity
	float nearPlane = maxExtent;
	float farPlane = 2.0f * maxExtent;
	glm::mat4 light_projection = glm::ortho(-maxExtent, maxExtent, -maxExtent, maxExtent, nearPlane, farPlane);

	// Combine view and projection matrices
	return light_projection * light_view;
}
