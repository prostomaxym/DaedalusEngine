#pragma once

#include <Daedalus.h>
#include <Daedalus/Config/KeybindConfig.h>

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
			auto camera = m_entity.GetComponent<CameraComponent>().camera.get();
			const auto pos = camera->GetPosition();
			const auto message = std::string("Position - ") + "X: " + std::to_string(pos.x) + " / " + "Y: " + std::to_string(pos.y) + " / " + "Z: " + std::to_string(pos.z);
			Log::Write(Log::Levels::Trace, Log::Categories::Application, message);
		}
	};

	class TankMovementScript : public NativeScript
	{
	public:
		TankMovementScript(Entity entity, glm::vec3 forward_dir, float distance) : NativeScript(entity), m_forward_dir(forward_dir), m_max_distance(distance) {}

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
			UpdatePlayerView();
			UpdatePlayerModel(dt);
			UpdateSpotLight();
		}

	private:
		bool m_spotlight_enabled{ true };
		bool m_thirdperson{ true };
		const float m_turn_speed = 540.f;
		std::unique_ptr<RenderableObjectComponent> m_temp_component_storage;

		void UpdateSpotLight()
		{
			const auto key_pressed = Input::IsKeyReleased(KeybindConfig::GetKeyboardBind("Flashlight"));

			if (key_pressed && m_spotlight_enabled)
			{
				m_spotlight_enabled = false;
				m_entity.RemoveComponent<SpotLightComponent>();
				
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
				m_entity.AddComponent<SpotLightComponent>(props);;
			}

			if (!m_entity.HasComponent<SpotLightComponent>())
				return;

			auto camera = m_entity.GetComponent<CameraComponent>().camera.get();
			auto& light = m_entity.GetComponent<SpotLightComponent>().light;

			if (!m_thirdperson)
			{
				light.SetPosition(camera->GetPosition());
				light.SetDirection(camera->GetDirection());
			}
			else
			{
				auto& trans = m_entity.GetComponent<TransformComponent>();

				light.SetPosition(camera->GetPosition() + camera->GetDirection());
				const auto yaw = camera->GetYaw();
				glm::vec3 front;
				front.x = std::cos(glm::radians(-trans.rotation.y - 90.f)) * std::cos(glm::radians(trans.rotation.x));
				front.y = std::sin(glm::radians(trans.rotation.x));
				front.z = std::sin(glm::radians(-trans.rotation.y - 90.f)) * std::cos(glm::radians(trans.rotation.x));
				front = glm::normalize(front);

				light.SetDirection(-front);
			}
		}

		void UpdatePlayerModel(DeltaTime dt)
		{
			auto camera = m_entity.GetComponent<CameraComponent>().camera.get();
			auto& trans = m_entity.GetComponent<TransformComponent>();
			trans.translation = camera->GetPosition();

			glm::vec3 camera_forward = glm::normalize(camera->GetDirection());
			glm::vec3 camera_right = glm::normalize(camera->GetRight());

			glm::vec3 moveDirection(0.0f);

			if (Input::IsKeyHold(KeybindConfig::GetKeyboardBind("MoveForward")))
			{
				moveDirection += camera_forward;
			}
			if (Input::IsKeyHold(KeybindConfig::GetKeyboardBind("MoveBackward")))
			{
				moveDirection -= camera_forward;
			}
			if (Input::IsKeyHold(KeybindConfig::GetKeyboardBind("MoveLeft")))
			{
				moveDirection -= camera_right;
			}
			if (Input::IsKeyHold(KeybindConfig::GetKeyboardBind("MoveRight")))
			{
				moveDirection += camera_right;
			}

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
				float currentYaw = trans.rotation.y;
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

				trans.rotation.y = newYaw;
			}
		}

		void UpdatePlayerView()
		{
			const auto key_pressed = Input::IsKeyReleased(KeybindConfig::GetKeyboardBind("ChangePOV"));
			auto camera = m_entity.GetComponent<CameraComponent>().camera.get();

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