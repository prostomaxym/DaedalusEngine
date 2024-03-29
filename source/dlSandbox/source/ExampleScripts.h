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
			auto& camera = m_entity.GetComponent<CameraComponent>().camera;
			const auto pos = camera.GetPosition();
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


	class MovingSpotLightScript : public NativeScript
	{
	public:
		MovingSpotLightScript(Entity entity) : NativeScript(entity) {}

	protected:

		virtual void OnUpdate(DeltaTime dt) override
		{
			auto& camera = m_entity.GetComponent<CameraComponent>().camera;
			auto& light = m_entity.GetComponent<SpotLightComponent>().light;
			light.SetPosition(camera.GetPosition());
			light.SetDirection(camera.GetDirection());
		}
	};
}