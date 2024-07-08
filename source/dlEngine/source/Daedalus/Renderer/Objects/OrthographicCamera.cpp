#include "dlpch.h"
#include "OrthographicCamera.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace Daedalus;


OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top, float front, float back,
	CameraPositionProps pos_props)
	: Camera(pos_props)
	, m_left_plane(left)
	, m_right_plane(right)
	, m_bottom_plane(bottom)
	, m_top_plane(top)
	, m_near_plane(front)
	, m_far_plane(back)
{
}

glm::mat4 OrthographicCamera::GetProjectionMatrix() const
{
	return glm::ortho(m_left_plane, m_right_plane,
		m_bottom_plane, m_top_plane,
		m_near_plane, m_far_plane);
}

glm::mat4 OrthographicCamera::GetProjectionMatrix(float z_far_limit) const
{
	return glm::ortho(m_left_plane, m_right_plane,
		m_bottom_plane, m_top_plane,
		m_near_plane, m_far_plane * z_far_limit);
}

void OrthographicCamera::SetProjection(float left, float right, float bottom, float top, float front, float back)
{
	m_left_plane = left;
	m_right_plane = right;
	m_bottom_plane = bottom;
	m_top_plane = top;
	m_near_plane = front;
	m_far_plane = back;
}

void OrthographicCamera::ProcessZoom(float offset)
{
	m_zoom = std::clamp(m_zoom + offset * m_zoom_speed, 0.2f, 5.f);
	m_left_plane /= m_zoom;
	m_right_plane /= m_zoom;
	m_bottom_plane /= m_zoom;
	m_top_plane /= m_zoom;
	m_near_plane *= m_zoom;
	m_far_plane *= m_zoom;
}