#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Daedalus {

	enum class DAEDALUS_API CameraMovement
	{
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT,
		UP,
		DOWN
	};

	struct DAEDALUS_API CameraProjectionProps
	{
		float fov = 70.f;
		float aspect_ratio = 16.f / 9.f;
		float z_near = 0.1f;
		float z_far = 10.f;

		CameraProjectionProps(float FOV = 70.f, float Aspect_ratio = 1.7777f, float Z_near = 0.1f, float Z_far = 10.f) :
			fov(FOV)
			, aspect_ratio(Aspect_ratio)
			, z_near(Z_near)
			, z_far(Z_far)
		{
		}
	};

	struct DAEDALUS_API CameraPositionProps
	{
		glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		float yaw = 0.f;
		float pitch = 0.f;

		CameraPositionProps(glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f), float Yaw = 0.f, float Pitch = 0.f) :
			position(Position)
			, up(Up)
			, yaw(Yaw)
			, pitch(Pitch)
		{
		}
	};

	class DAEDALUS_API PerspectiveCamera
	{
	public:
		PerspectiveCamera(CameraProjectionProps proj_props = CameraProjectionProps(), CameraPositionProps pos_props = CameraPositionProps());

		glm::mat4 GetViewMatrix() const;
		glm::mat4 GetProjectionMatrix() const;
		glm::mat4 GetProjectionViewMatrix() const;

		void SetPosition(glm::vec3 position);
		void SetRotation(float xoffset, float yoffset);

		glm::vec3 GetPosition() { return m_position; }

		void ProcessKeyboard(CameraMovement direction, float delta_time);
		void ProcessMouseMovement(float xoffset, float yoffset, bool constrain_pitch = true);
		void ProcessMouseScroll(float yoffset);

		void MoveCamera(CameraMovement direction, float distance);
		void RotateCamera(float xoffset, float yoffset);

	private:
		void Update();

		CameraProjectionProps m_proj_props;

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