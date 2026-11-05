#pragma once

#include <Daedalus.h>

#include "ExampleLayer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

namespace Daedalus
{
	// Just some examples of basic scripts

	class RotationModelScript : public NativeScript
	{
	public:
		RotationModelScript(Entity entity, float speed) : NativeScript(entity), m_speed(speed) {}
		float GetSpeed() const { return m_speed; }

	protected:

		virtual void OnUpdate(DeltaTime dt) override
		{
			m_entity.GetComponent<TransformComponent>().rotation.y += m_speed * dt.GetMilliseconds();
		}

		float m_speed{ 0.f };
	};

	class LogPositionScript : public NativeScript
	{
	public:
		LogPositionScript(Entity entity) : NativeScript(entity) {}

	protected:

		virtual void OnUpdate(DeltaTime dt) override
		{
			auto entity = m_entity.HasComponent<CameraComponent>() ? m_entity : m_entity.GetLinkedEntity("camera");
			if (!entity) return;
			entity.GetScene().UpdateWorldTransforms();
			auto camera = entity.GetComponent<CameraComponent>().camera.get();
			const auto pos = camera->GetPosition();
			const auto message = std::string("Position - ") + "X: " + std::to_string(pos.x) + " / " + "Y: " + std::to_string(pos.y) + " / " + "Z: " + std::to_string(pos.z);
			Log::Write(Log::Levels::Trace, Log::Categories::Application, "{}", message);
		}
	};

	class TankMovementScript : public NativeScript
	{
	public:
		TankMovementScript(Entity entity, glm::vec3 forward_dir, float distance) : NativeScript(entity), m_forward_dir(forward_dir), m_max_distance(distance) {}
		glm::vec3 GetForwardDirection() const { return m_forward_dir; }
		float GetMaxDistance() const { return m_max_distance; }

	protected:

		virtual void OnUpdate(DeltaTime dt) override
		{
			auto& transform = m_entity.GetComponent<TransformComponent>();

			auto delta_movement = dt.GetSeconds() * 5.f;
			auto delta_rotation = dt.GetSeconds() * 180.f * 0.3f;

			if (m_current_length_pos <= m_max_distance && !is_rotating && moving_upwards)
			{
				delta_movement = std::min(delta_movement, m_max_distance - m_current_length_pos);
				m_current_length_pos += delta_movement;
				transform.translation += delta_movement * m_forward_dir;
				if (m_current_length_pos >= m_max_distance)
				{
					is_rotating = true;
				}
			}
			else if (m_current_length_pos >= 0 && !is_rotating && !moving_upwards)
			{
				delta_movement = std::min(delta_movement, m_current_length_pos);
				m_current_length_pos -= delta_movement;
				transform.translation -= delta_movement * m_forward_dir;
				if (m_current_length_pos <= 0.f)
				{
					is_rotating = true;
				}
			}

			if (m_current_length_pos >= m_max_distance && is_rotating)
			{
				delta_rotation = std::min(delta_rotation, 180.f - m_current_angle);
				m_current_angle += delta_rotation;
				transform.rotation.y -= delta_rotation;
				if (m_current_angle >= 180.f)
				{
					is_rotating = false;
					moving_upwards = false;
				}
			}
			else if (m_current_length_pos <= 0.f && is_rotating)
			{
				delta_rotation = std::min(delta_rotation, m_current_angle);
				m_current_angle -= delta_rotation;
				transform.rotation.y += delta_rotation;
				if (m_current_angle <= 0.f)
				{
					is_rotating = false;
					moving_upwards = true;
				}
			}
		}

		glm::vec3 m_forward_dir;
		float m_current_length_pos{ 0.f };
		float m_current_angle{ 0.f };
		float m_max_distance{ 0.f };
		bool is_rotating{ false };
		bool moving_upwards{ true };
	};


	class PlayerControllerScript : public NativeScript
	{
	public:
		PlayerControllerScript(Entity entity) : NativeScript(entity) {}

	protected:
		virtual void OnUpdate(DeltaTime dt) override
		{
			if (!GetCameraEntity()) return;
			m_entity.GetScene().UpdateWorldTransforms();
			UpdatePlayerView();
			UpdatePlayerModel(dt);
			UpdateSpotLight();
			m_entity.GetScene().UpdateWorldTransforms();
		}

	private:
		bool m_spotlight_enabled{ true };
		bool m_thirdperson{ true };
		const float m_turn_speed = 540.f;
		std::unique_ptr<RenderableObjectComponent> m_temp_component_storage;

		Entity GetCameraEntity() const
		{
			return m_entity.HasComponent<CameraComponent>() ? m_entity : m_entity.GetLinkedEntity("camera");
		}

		void UpdateSpotLight()
		{
			auto entity = m_entity.GetLinkedEntity("spot_light");
			if (!entity) entity = m_entity;
			const auto key_pressed = Input::IsActionReleased("Flashlight");

			if (key_pressed && m_spotlight_enabled)
			{
				m_spotlight_enabled = false;
				if (entity.HasComponent<SpotLightComponent>()) entity.RemoveComponent<SpotLightComponent>();
				
			}
			else if (key_pressed)
			{
				m_spotlight_enabled = true;

				Daedalus::LightProps props;
				props.light_pos = glm::vec3(0.f, 0.f, 0.f);
				props.ambient_color = glm::vec3(0.f, 0.f, 0.f);
				props.diffuse_color = glm::vec3(1.f, 1.f, 1.f);
				props.specular_color = glm::vec3(1.f, 1.f, 1.f);
				props.direction = glm::vec3(0.f, 0.f, -1.f);
				props.light_power = 4.f;
				props.max_distance = 100.f;
				props.theta_angle = 10.f;
				props.outer_cutoff = 35.f;
				props.cast_shadows = false;
				props.number_of_shadow_cascades = 1;
				entity.AddComponent<SpotLightComponent>(props);
			}

			if (!entity.HasComponent<SpotLightComponent>())
				return;
			if (entity == m_entity) return;

			auto camera = GetCameraEntity().GetComponent<CameraComponent>().camera.get();

			if (!m_thirdperson)
			{
				(void)entity.GetScene().SetWorldPose(entity.GetUUID(), camera->GetPosition(), camera->GetOrientation());
			}
			else
			{
				const auto orientation = m_entity.GetScene().GetWorldOrientation(m_entity.GetUUID())
					* glm::angleAxis(glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f));
				(void)entity.GetScene().SetWorldPose(entity.GetUUID(), camera->GetPosition() + camera->GetDirection(), orientation);
			}
		}

		void UpdatePlayerModel(DeltaTime dt)
		{
			auto camera = GetCameraEntity().GetComponent<CameraComponent>().camera.get();
			auto& trans = m_entity.GetComponent<TransformComponent>();
			const auto parent = m_entity.GetScene().GetParent(m_entity.GetUUID());
			const auto parent_rotation = m_entity.GetScene().GetWorldOrientation(parent);
			(void)m_entity.GetScene().SetWorldPose(m_entity.GetUUID(), camera->GetPosition(), parent_rotation * trans.GetOrientationQuat());

			glm::vec3 camera_forward = glm::normalize(camera->GetDirection());
			glm::vec3 camera_right = glm::normalize(camera->GetRight());

			const auto forward = Input::GetMovementAction("MoveForward", "MoveBackward");
			const auto right = Input::GetMovementAction("MoveRight", "MoveLeft");
			glm::vec3 moveDirection = glm::inverse(parent_rotation) * (camera_forward * forward + camera_right * right);
			// Facing is yaw-only in parent space, independent of camera pitch.
			moveDirection.y = 0.f;

			if (glm::length(moveDirection) > 0.0f)
			{
				// Generated by ChatGPT - have not time to rewrite it human readable way
				moveDirection = glm::normalize(moveDirection);

				// Calculate yaw from the camera's forward direction
				float desiredYaw = glm::degrees(atan2(moveDirection.x, moveDirection.z));

				// Normalize the desiredYaw to the range [0, 360]
				if (desiredYaw < 0)
				{
					desiredYaw += 360.0f;
				}

				// Calculate the difference and adjust for shortest path
				// Quaternion-to-Euler writeback can represent yaw > 90 with 180-degree pitch/roll.
				const auto model_forward = trans.GetOrientationQuat() * glm::vec3(0.f, 0.f, 1.f);
				float currentYaw = glm::degrees(atan2(model_forward.x, model_forward.z));
				if (currentYaw < 0.f) currentYaw += 360.f;
				float deltaYaw = desiredYaw - currentYaw;

				if (deltaYaw > 180.0f)
				{
					deltaYaw -= 360.0f;
				}
				else if (deltaYaw < -180.0f)
				{
					deltaYaw += 360.0f;
				}

				// Gradual rotation using linear interpolation
				float rotationSpeed = m_turn_speed * dt.GetSeconds(); // degrees per second, adjust as needed
				float newYaw = currentYaw + glm::clamp(deltaYaw, -rotationSpeed, rotationSpeed);

				// Normalize the newYaw to the range [0, 360]
				if (newYaw < 0)
				{
					newYaw += 360.0f;
				}
				else if (newYaw >= 360.0f)
				{
					newYaw -= 360.0f;
				}

				trans.rotation = glm::vec3(0.f, newYaw, 0.f);
			}
			m_entity.GetScene().UpdateWorldTransforms();
		}

		void UpdatePlayerView()
		{
			const auto key_pressed = Input::IsActionReleased("ChangePOV");
			auto camera = GetCameraEntity().GetComponent<CameraComponent>().camera.get();

			if (key_pressed && m_thirdperson)
			{
				m_thirdperson = false;
				m_temp_component_storage = m_entity.PopComponent<RenderableObjectComponent>();
				camera->SetPersonParams(0.f, 0.f);
			}
			else if (key_pressed)
			{
				m_thirdperson = true;
				m_entity.PushComponent<RenderableObjectComponent>(std::move(m_temp_component_storage));
				camera->SetPersonParams(7.f, 4.f);
			}
		}
	};
}