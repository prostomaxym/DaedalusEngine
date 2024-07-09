#include "PerspectiveCamera.h"

using namespace Daedalus;

PerspectiveCamera::PerspectiveCamera(CameraProjectionProps proj_props, CameraPositionProps pos_props) :
	Camera(pos_props)
	, m_proj_props(proj_props)
{
}

glm::mat4 PerspectiveCamera::GetProjectionMatrix() const
{
	return glm::perspective(glm::radians(m_proj_props.fov), m_proj_props.aspect_ratio, m_proj_props.z_near, m_proj_props.z_far);
}

glm::mat4 PerspectiveCamera::GetProjectionMatrix(float z_far_limit) const
{
	return glm::perspective(glm::radians(m_proj_props.fov), m_proj_props.aspect_ratio, m_proj_props.z_near, m_proj_props.z_far * z_far_limit);
}

glm::mat4 PerspectiveCamera::GetProjectionMatrix(float z_close_limit, float z_far_limit) const
{
	return glm::perspective(glm::radians(m_proj_props.fov), m_proj_props.aspect_ratio, m_proj_props.z_far * z_close_limit, m_proj_props.z_far * z_far_limit);
}

glm::vec3 PerspectiveCamera::GetCascadeDistances(float perc_low, float perc_mid, float perc_high) const
{
	return glm::vec3(m_proj_props.z_far * perc_low, m_proj_props.z_far * perc_mid, m_proj_props.z_far * perc_high);
}
void PerspectiveCamera::ProcessZoom(float offset)
{
	m_zoom = std::clamp(m_zoom + offset * m_zoom_speed, 0.2f, 5.f);
	m_proj_props.fov *= m_zoom;
}