#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Daedalus {

enum CameraMovement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

class PerspectiveCamera
{
public:
    PerspectiveCamera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.f, float pitch = 0.f);
    PerspectiveCamera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

    glm::mat4 GetViewMatrix();

    void ProcessKeyboard(CameraMovement direction, float delta_time);
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrain_pitch = true);
    void ProcessMouseScroll(float yoffset);

private:
    void Update();

    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_world_up;

    float m_yaw = -90.f;
    float m_pitch = 0.f;

    float m_movement_speed = 3.f;
    float m_mouse_sensitivity = 0.25f;
    float m_zoom = 45.f;
};

}