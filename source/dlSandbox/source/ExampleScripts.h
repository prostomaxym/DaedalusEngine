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
		TankMovementScript(Entity entity) : NativeScript(entity) {}

	protected:

		virtual void OnUpdate(DeltaTime dt) override
		{
			auto& transform = m_entity.GetComponent<TransformComponent>();

			auto delta_movement = dt.GetSeconds() * 5.f;
			auto delta_rotation = dt.GetSeconds() * 180.f * 0.3f;

			if (m_current_length_pos <= 50.f && !is_rotating && moving_upwards)
			{
				delta_movement = std::min(delta_movement, 50.f - m_current_length_pos);
				m_current_length_pos += delta_movement;
				transform.translation.x += delta_movement;
				if (m_current_length_pos >= 50.f)
				{
					is_rotating = true;
				}
			}
			else if (m_current_length_pos >= 0 && !is_rotating && !moving_upwards)
			{
				delta_movement = std::min(delta_movement, m_current_length_pos);
				m_current_length_pos -= delta_movement;
				transform.translation.x -= delta_movement;
				if (m_current_length_pos <= 0.f)
				{
					is_rotating = true;
				}
			}

			if (m_current_length_pos >= 50.f && is_rotating)
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

		bool is_rotating{ false };
		bool moving_upwards{ true };
		float m_current_length_pos{ 0.f };
		float m_current_angle{ 0.f };
	};
}