#include "dlpch.h"

#include "SpotLightSource.h"

using namespace Daedalus;

SpotLightSource::SpotLightSource(const LightProps& props) :
    LightSource(LightSourceType::Spot, props)
{
	DL_ASSERT(m_params.number_of_cascades <= 1, Log::Categories::Renderer, "Spot Light CSM is not supported now");
	m_params.number_of_cascades = 1;
}

glm::mat4 SpotLightSource::CalculateProjViewForFrustum(const glm::mat4& camera_proj, const glm::mat4&) const
{
	const auto light_view = glm::lookAt(
		m_params.position,
		m_params.position + m_params.direction,
		glm::vec3(0.0f, 1.0f, 0.0f)
	);

	float fov = 0.f, aspect = 0.f, znear = 0.f, zfar = 0.f;
	ExtractPerspectiveParams(camera_proj, fov, aspect, znear, zfar);

	// TODO fix for orthographic camera
	const glm::mat4 light_projection = glm::perspective(fov, 1.f, znear, zfar);//std::min(zfar, m_max_distance));

	return light_projection * light_view;
}