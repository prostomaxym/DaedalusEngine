#include "dlpch.h"
#include "OrthographicCamera.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace Daedalus;


OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top, float front, float back,
	CameraPositionProps pos_props)
	: Camera(pos_props)
{
}

glm::mat4 OrthographicCamera::GetProjectionMatrix() const
{
	return glm::ortho(m_left, m_right,
		m_bottom, m_top,
		m_front, m_back);
}

glm::mat4 OrthographicCamera::GetProjectionMatrix(float z_far_limit) const
{
	return glm::ortho(m_left, m_right,
		m_bottom, m_top,
		m_front, m_back * z_far_limit);
}

void OrthographicCamera::SetProjection(float left, float right, float bottom, float top, float front, float back)
{
	m_left = left;
	m_right = right;
	m_bottom = bottom;
	m_top = top;
	m_front = top;
	m_back = back;
}