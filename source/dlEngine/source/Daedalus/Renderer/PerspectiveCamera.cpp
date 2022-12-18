#pragma once

#include "PerspectiveCamera.h"

using namespace Daedalus;

PerspectiveCamera::PerspectiveCamera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) :
    m_front(glm::vec3(0.0f, 0.0f, -1.0f))
    , m_movement_speed(3.f)
    , m_mouse_sensitivity(0.25f)
    , m_zoom(45.f)
{
    m_position = position;
    m_world_up = up;
    m_yaw = yaw;
    m_pitch = pitch;

    Update();
}

PerspectiveCamera::PerspectiveCamera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) :
    m_front(glm::vec3(0.0f, 0.0f, -1.0f))
    , m_movement_speed(3.f)
    , m_mouse_sensitivity(0.25f)
    , m_zoom(45.f)
{
    m_position = glm::vec3(posX, posY, posZ);
    m_world_up = glm::vec3(upX, upY, upZ);
    m_yaw = yaw;
    m_pitch = pitch;

    Update();
}

glm::mat4 PerspectiveCamera::GetViewMatrix()
{
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

void PerspectiveCamera::ProcessKeyboard(CameraMovement direction, float delta_time)
{
    float velocity = m_movement_speed * delta_time;
    if (direction == FORWARD)
        m_position += m_front * velocity;
    if (direction == BACKWARD)
        m_position -= m_front * velocity;
    if (direction == LEFT)
        m_position -= m_right * velocity;
    if (direction == RIGHT)
        m_position += m_right * velocity;
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

void PerspectiveCamera::ProcessMouseScroll(float yoffset)
{
    if (m_zoom >= 1.0f && m_zoom <= 45.0f)
        m_zoom -= yoffset;
    if (m_zoom <= 1.0f)
        m_zoom = 1.0f;
    if (m_zoom >= 45.0f)
        m_zoom = 45.0f;
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
