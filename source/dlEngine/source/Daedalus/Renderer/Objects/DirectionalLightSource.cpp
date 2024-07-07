#include "dlpch.h"

#include "DirectionalLightSource.h"
#include "Daedalus/Renderer/API/RenderConstants.h"
#include "Geometry.h"

using namespace Daedalus;

DirectionalLightSource::DirectionalLightSource(
    glm::vec3 light_direction
    , glm::vec3 ambient_color
    , glm::vec3 diffuse_color
    , glm::vec3 specular_color
    , bool cast_shadow
    , float light_power) :
    m_params(0, light_direction, ambient_color, diffuse_color, specular_color, cast_shadow, light_power, 100.f, light_direction)
{}

void DirectionalLightSource::UpdateSSBOForViewFrustum(const glm::mat4& proj, const glm::mat4& view)
{
	m_params.proj_view = CalculateLightMatrixForFrustum(proj, view);
}

void DirectionalLightSource::UpdateSSBOForSceneFrustum(const BoundingSphere& sphere)
{
	m_params.proj_view = CalculateLightMatrixForSphere(sphere);
}

glm::mat4 DirectionalLightSource::CalculateLightMatrixForFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view) const
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
		center + m_params.direction,
		center,
		glm::vec3(0.0f, 1.0f, 0.0f)
	);

	float minX = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::lowest();
	float minY = std::numeric_limits<float>::max();
	float maxY = std::numeric_limits<float>::lowest();
	float minZ = std::numeric_limits<float>::max();
	float maxZ = std::numeric_limits<float>::lowest();

	for (const auto& v : frustum_corners)
	{
		const auto trf = light_view * v;
		minX = std::min(minX, trf.x);
		maxX = std::max(maxX, trf.x);
		minY = std::min(minY, trf.y);
		maxY = std::max(maxY, trf.y);
		minZ = std::min(minZ, trf.z);
		maxZ = std::max(maxZ, trf.z);
	}

	// Tune this parameter according to the scene
	constexpr float xMult = 1.2f;
	constexpr float yMult = 1.2f;
	constexpr float zMult = 1.2f;

	if (minX < 0)
		minX *= xMult;
	else
		minX /= xMult;

	if (maxX < 0)
		maxX /= xMult;
	else
		maxX *= xMult;

	if (minY < 0)
		minY *= yMult;
	else
		minY /= yMult;

	if (maxY < 0)
		maxY /= yMult;
	else
		maxY *= yMult;

	if (minZ < 0)
		minZ *= zMult;
	else
		minZ /= zMult;

	if (maxZ < 0)
		maxZ /= zMult;
	else
		maxZ *= zMult;

	const glm::mat4 light_projection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
	return light_projection * light_view;
}

glm::mat4 DirectionalLightSource::CalculateLightMatrixForSphere(const BoundingSphere& sphere) const
{
	const auto light_view = glm::lookAt(
		sphere.position + m_params.direction,
		sphere.position,
		glm::vec3(0.0f, 1.0f, 0.0f)
	);

	const glm::mat4 light_projection = glm::ortho(
		sphere.position.x - sphere.radius, sphere.position.x + sphere.radius,
		sphere.position.y - sphere.radius, sphere.position.y + sphere.radius,
		sphere.position.z - sphere.radius, sphere.position.z + sphere.radius);

	return light_projection * light_view;
}
