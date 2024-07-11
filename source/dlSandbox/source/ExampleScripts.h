#pragma once

#include <Daedalus.h>

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

	private:
		bool m_spotlight_enabled{ true };
		bool m_thirdperson{ true };
		const float turn_speed = 10.f;

		void UpdateSpotLight()
		{
			const auto key_pressed = Input::IsKeyReleased(DL_KEY_F);

			if (key_pressed && m_spotlight_enabled)
			{
				m_spotlight_enabled = false;
				m_entity.RemoveComponent<SpotLightComponent>();
			}
			else if (key_pressed && !m_spotlight_enabled)
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
				m_entity.AddComponent<SpotLightComponent>(props);
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
				light.SetDirection(trans.rotation);
			}

		}
		void UpdatePlayerModel(DeltaTime dt)
		{
			auto camera = m_entity.GetComponent<CameraComponent>().camera.get();
			auto& trans = m_entity.GetComponent<TransformComponent>();
			trans.translation = camera->GetPosition();

			glm::vec3 camera_forward = camera->GetDirection();
			glm::vec3 camera_right = camera->GetRight();
			glm::vec3 camera_up = camera->GetUp();

			glm::quat rotation;
			bool key_pressed = false;

			if (Input::IsKeyHold(DL_KEY_W))
			{
				rotation = glm::quatLookAt(-camera_forward, camera_up); // Facing the camera
				key_pressed = true;
			}
			if (Input::IsKeyHold(DL_KEY_S))
			{
				rotation = glm::quatLookAt(camera_forward, camera_up); // Facing away from the camera
				key_pressed = true;
			}

			if (Input::IsKeyHold(DL_KEY_A))
			{
				rotation = glm::quatLookAt(camera_right, camera_up); // Facing right of the camera
				key_pressed = true;
			}

			if (Input::IsKeyHold(DL_KEY_D)* dt.GetSeconds())
			{
				rotation = glm::quatLookAt(-camera_right, camera_up); // Facing left of the camera
				key_pressed = true;
			}

			if(key_pressed)
				trans.rotation = glm::degrees(glm::eulerAngles(rotation) * dt.GetSeconds());
		}

	protected:

		virtual void OnUpdate(DeltaTime dt) override
		{
			UpdatePlayerModel(dt);
			UpdateSpotLight();
		}
	};
}