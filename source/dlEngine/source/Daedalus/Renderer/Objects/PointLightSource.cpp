#include "dlpch.h"

#include "PointLightSource.h"

using namespace Daedalus;

PointLightSource::PointLightSource(const LightProps& props)
	: 
	LightSource(LightSourceType::Point, props)
{}

std::vector<glm::mat4> PointLightSource::CalculateShadowCubemapProjView(const glm::mat4& camera_proj, const glm::mat4& camera_view) const
{
	float fov = 0.f, aspect = 0.f, znear = 0.f, zfar = 0.f;
	ExtractPerspectiveParams(camera_proj, fov, aspect, znear, zfar);
	const glm::mat4 shadow_proj = glm::perspective(fov, 1.f, znear, zfar);//std::min(zfar, m_max_distance));

    std::vector<glm::mat4> shadow_transforms;

	shadow_transforms.push_back(shadow_proj *
				 glm::lookAt(m_params.position, m_params.position + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
	shadow_transforms.push_back(shadow_proj *
					 glm::lookAt(m_params.position, m_params.position + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
	shadow_transforms.push_back(shadow_proj *
					 glm::lookAt(m_params.position, m_params.position + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
	shadow_transforms.push_back(shadow_proj *
					 glm::lookAt(m_params.position, m_params.position + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0)));
	shadow_transforms.push_back(shadow_proj *
					 glm::lookAt(m_params.position, m_params.position + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0)));
	shadow_transforms.push_back(shadow_proj *
					 glm::lookAt(m_params.position, m_params.position + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0)));

	return shadow_transforms;
}