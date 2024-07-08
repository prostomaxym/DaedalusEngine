#pragma once

#include "Geometry.h"

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

	class DAEDALUS_API Camera
	{
	public:
		explicit Camera(CameraPositionProps pos_props = CameraPositionProps());
		virtual ~Camera() = default;

		Frustum GetViewFrustum() const;

		glm::mat4 GetViewMatrix() const;
		virtual glm::mat4 GetProjectionMatrix() const = 0;
		virtual glm::mat4 GetProjectionMatrix(float z_far_limit) const = 0; // For quick calculations. E.G take 0.2 of max camera distance

		glm::mat4 GetViewProjectionMatrix() const;
		glm::mat4 GetProjectionViewMatrix() const;
		glm::mat4 GetProjectionViewMatrixWithoutTranslation(float rotate_angle) const;

		glm::vec3 GetDirection() const { return m_front; }
		glm::vec3 GetUp() const { return m_up; }
		glm::vec3 GetPosition() const { return m_position; }

		void SetPosition(glm::vec3 position);
		void SetRotation(float xoffset, float yoffset);
		void SetMovementSpeed(float speed) { m_movement_speed = speed; }
		void SetSensivitity(float senc) { m_mouse_sensitivity = senc; }
		void SetZoomSpeed(float speed) { m_zoom_speed = speed; }

		void ProcessKeyboard(CameraMovement direction, float delta_time);
		void ProcessMouseMovement(float xoffset, float yoffset, bool constrain_pitch = true);
		virtual void ProcessZoom(float yoffset) = 0;

		void MoveCamera(CameraMovement direction, float distance);
		void RotateCamera(float xoffset, float yoffset);

	protected:
		void UpdateView();

		glm::vec3 m_position;
		glm::vec3 m_front;
		glm::vec3 m_up;
		glm::vec3 m_right;
		glm::vec3 m_world_up;

		float m_yaw = -90.f;
		float m_pitch = 0.f;

		float m_movement_speed = 3.f;
		float m_mouse_sensitivity = 2.f;
		float m_zoom = 1.f;
		float m_zoom_speed = 1.f;
	};
}