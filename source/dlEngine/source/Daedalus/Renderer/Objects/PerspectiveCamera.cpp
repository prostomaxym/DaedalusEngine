#pragma once

#include "PerspectiveCamera.h"

using namespace Daedalus;

PerspectiveCamera::PerspectiveCamera(CameraProjectionProps proj_props, CameraPositionProps pos_props) :
	m_proj_props(proj_props)
	, m_position(pos_props.position)
	, m_world_up(pos_props.up)
	, m_yaw(pos_props.yaw)
	, m_pitch(pos_props.pitch)
{
	Update();
}

glm::mat4 PerspectiveCamera::GetViewMatrix() const
{
	return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 PerspectiveCamera::GetProjectionMatrix() const
{
	return glm::perspective(m_proj_props.fov, m_proj_props.aspect_ratio, m_proj_props.z_near, m_proj_props.z_far);
}

glm::mat4 PerspectiveCamera::GetProjectionViewMatrix() const
{
	return glm::perspective(m_proj_props.fov, m_proj_props.aspect_ratio, m_proj_props.z_near, m_proj_props.z_far) *
		glm::lookAt(m_position, m_position + m_front, m_up);
}

void PerspectiveCamera::SetPosition(glm::vec3 position)
{
	m_position = position;

	Update();
}

void PerspectiveCamera::SetRotation(float xoffset, float yoffset)
{
	m_yaw = xoffset;
	m_pitch = yoffset;
}

void PerspectiveCamera::ProcessKeyboard(CameraMovement direction, float delta_time)
{
	float velocity = m_movement_speed * delta_time;
	if (direction == CameraMovement::FORWARD)
		m_position += m_front * velocity;
	if (direction == CameraMovement::BACKWARD)
		m_position -= m_front * velocity;
	if (direction == CameraMovement::LEFT)
		m_position -= m_right * velocity;
	if (direction == CameraMovement::RIGHT)
		m_position += m_right * velocity;
	if (direction == CameraMovement::UP)
		m_position += m_world_up * velocity;
	if (direction == CameraMovement::DOWN)
		m_position -= m_world_up * velocity;
}

void PerspectiveCamera::ProcessMouseMovement(float xoffset, float yoffset, bool constrain_pitch)
{
	xoffset *= m_mouse_sensitivity;
	yoffset *= m_mouse_sensitivity;

	m_yaw += xoffset;
	m_pitch += yoffset;

	if (constrain_pitch)
	{
		if (m_pitch > 89.0f)
			m_pitch = 89.0f;
		if (m_pitch < -89.0f)
			m_pitch = -89.0f;
	}

	Update();
}

void PerspectiveCamera::ProcessZoom(float offset)
{
	m_zoom = std::clamp(m_zoom + offset * m_zoom_speed, 1.f, 2.f);
	m_proj_props.fov *= m_zoom;
}

void PerspectiveCamera::MoveCamera(CameraMovement direction, float distance)
{
	if (direction == CameraMovement::FORWARD)
		m_position += m_front * distance;
	if (direction == CameraMovement::BACKWARD)
		m_position -= m_front * distance;
	if (direction == CameraMovement::LEFT)
		m_position -= m_right * distance;
	if (direction == CameraMovement::RIGHT)
		m_position += m_right * distance;
	if (direction == CameraMovement::UP)
		m_position += m_world_up * distance;
	if (direction == CameraMovement::DOWN)
		m_position -= m_world_up * distance;
}

void PerspectiveCamera::RotateCamera(float xoffset, float yoffset)
{
	m_yaw += xoffset;
	m_pitch += yoffset;

	Update();
}

void PerspectiveCamera::Update()
{
	glm::vec3 front;
	front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	front.y = sin(glm::radians(m_pitch));
	front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));

	m_front = glm::normalize(front);
	m_right = glm::normalize(glm::cross(m_front, m_world_up));
	m_up = glm::normalize(glm::cross(m_right, m_front));
}
