#include "Camera.h"

using namespace Daedalus;

Camera::Camera(CameraPositionProps pos_props) :
	m_position(pos_props.position)
	, m_world_up(pos_props.up)
	, m_yaw(pos_props.yaw)
	, m_pitch(pos_props.pitch)
{
}

Frustum Camera::GetViewFrustum() const
{
	return Frustum::CalculateFrustum(GetProjectionViewMatrix());
}

glm::mat4 Camera::GetViewMatrix() const
{
	glm::vec3 position = m_position;
	position.y += m_height;
	position += m_front * (-m_distance);

	return glm::lookAt(position, m_position + m_front, m_up);
}

glm::mat4 Camera::GetProjectionViewMatrix() const
{
	return GetProjectionMatrix() * GetViewMatrix();
}

glm::mat4 Camera::GetViewProjectionMatrix() const
{
	return GetViewMatrix() * GetProjectionMatrix();
}

glm::mat4 Camera::GetProjectionViewMatrixWithoutTranslation(float rotate_angle) const
{
	glm::mat4 rotation_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotate_angle), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 rotated_vector = glm::vec3(rotation_matrix * glm::vec4(m_front, 0.0f));
	glm::mat4 view_matrix_without_translation = glm::mat4(glm::mat3(glm::lookAt(m_position, m_position + rotated_vector, m_up)));

	return GetProjectionMatrix() * view_matrix_without_translation;
}

void Camera::SetPosition(glm::vec3 position)
{
	m_position = position;

	UpdateView();
}

void Camera::SetPersonParams(float distance, float height)
{
	m_distance = distance;
	m_height = height;
}

void Camera::SetRotation(float xoffset, float yoffset)
{
	m_yaw = xoffset;
	m_pitch = yoffset;

	UpdateView();
}

void Camera::ProcessKeyboard(CameraMovement direction, float delta_time)
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

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrain_pitch)
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

	UpdateView();
}

void Camera::MoveCamera(CameraMovement direction, float distance)
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

void Camera::RotateCamera(float xoffset, float yoffset)
{
	m_yaw += xoffset;
	m_pitch += yoffset;

	UpdateView();
}

void Camera::UpdateView()
{
	glm::vec3 front;
	front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
	front.y = std::sin(glm::radians(m_pitch));
	front.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));

	m_front = glm::normalize(front);
	m_right = glm::normalize(glm::cross(m_front, m_world_up));
	m_up = glm::normalize(glm::cross(m_right, m_front));
}