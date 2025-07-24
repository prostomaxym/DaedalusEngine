#include "dlpch.h"

#include "DirectionalLightSource.h"
#include "Renderer/API/RenderConstants.h"
#include "Geometry.h"

using namespace Daedalus;

namespace
{
	// Just magic number, needed for proper shadow cascades
	// have no idea how to get rid of it
	const auto zMultMagicNum = 5.f;
}

DirectionalLightSource::DirectionalLightSource(const LightProps& props) :
    LightSource(LightSourceType::Directional, props)
{}

glm::mat4 DirectionalLightSource::CalculateProjViewForFrustum(const glm::mat4& camera_proj, const glm::mat4& camera_view) const
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

	if (minZ < 0)
		minZ *= zMultMagicNum;
	else
		minZ /= zMultMagicNum;

	if (maxZ < 0)
		maxZ /= zMultMagicNum;
	else
		maxZ *= zMultMagicNum;

	const glm::mat4 light_projection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
	return light_projection * light_view;
}

glm::mat4 DirectionalLightSource::CalculateProjViewForSphere(const BoundingSphere& sphere) const
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

glm::mat4 DirectionalLightSource::CalculateProjViewForAABB(const AABB& aabb) const
{
	const auto light_view = glm::lookAt(
		aabb.center + m_params.direction,
		aabb.center,
		glm::vec3(0.0f, 1.0f, 0.0f)
	);

	const auto min = aabb.GetMin();
	const auto max = aabb.GetMax();
	const glm::mat4 light_projection = glm::ortho(
		min.x, max.x,
		min.y, max.y,
		min.z, max.z);

	return light_projection * light_view;
}