#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace Daedalus
{
	class TouchControls
	{
	public:
		enum class Button { Up, Down, Console, Debug, POV, Flashlight, Count };
		static constexpr size_t ButtonCount = static_cast<size_t>(Button::Count);
		static constexpr std::array<const char*, ButtonCount> ButtonLabels{ "Up", "Down", "Console", "Debug", "POV", "Light" };

		void Resize(float width, float height)
		{
			if (width == m_width && height == m_height) return;
			Reset();
			m_width = width;
			m_height = height;
		}

		float Radius() const { return std::min(m_width, m_height) * 0.12f; }
		std::pair<float, float> MovementCenter() const { return { Radius() * 1.6f, m_height - Radius() * 1.6f }; }
		std::pair<float, float> UpCenter() const { return { m_width - Radius() * 1.4f, m_height - Radius() * 1.5f }; }
		std::pair<float, float> Movement() const { return m_movement; }
		bool Up() const { return Held(Button::Up); }
		bool Held(Button button) const { return m_buttons[static_cast<size_t>(button)].pointer >= 0; }
		bool ConsumeRelease(Button button) { return std::exchange(m_buttons[static_cast<size_t>(button)].released, false); }
		std::pair<float, float> ButtonCenter(Button button) const
		{
			if (button == Button::Up) return UpCenter();
			if (button == Button::Down) return { m_width - Radius() * 3.f, m_height - Radius() * 1.5f };
			return { m_width * (0.125f + 0.25f * (static_cast<float>(button) - 2.f)), Radius() * 0.85f };
		}
		float ButtonRadius(Button button) const { return Radius() * (button <= Button::Down ? 0.65f : 0.55f); }

		void Down(int id, float x, float y)
		{
			if (id < 0 || Radius() <= 0.f || id == m_move_id || id == m_look_id) return;
			for (const auto& button : m_buttons) if (button.pointer == id) return;
			for (size_t i = 0; i < ButtonCount; ++i)
			{
				const auto button = static_cast<Button>(i);
				const auto [bx, by] = ButtonCenter(button);
				if (std::hypot(x - bx, y - by) <= ButtonRadius(button))
				{
					if (m_buttons[i].pointer < 0) m_buttons[i].pointer = id;
					return;
				}
			}
			if (x < m_width * 0.45f)
			{
				if (m_move_id < 0) { m_move_id = id; Move(id, x, y); }
			}
			else if (m_look_id < 0)
			{
				m_look_id = id;
				m_last_look = { x, y };
			}
		}

		void Move(int id, float x, float y)
		{
			if (id < 0) return;
			if (id == m_move_id)
			{
				const auto [cx, cy] = MovementCenter();
				const float dx = (x - cx) / Radius(), dy = (y - cy) / Radius();
				const float length = std::max(1.f, std::hypot(dx, dy));
				m_movement = { dx / length, dy / length };
			}
			if (id == m_look_id)
			{
				m_look_delta.first += x - m_last_look.first;
				m_look_delta.second += y - m_last_look.second;
				m_last_look = { x, y };
			}
		}

		void Release(int id)
		{
			if (id < 0) return;
			if (id == m_move_id) { m_move_id = -1; m_movement = {}; }
			if (id == m_look_id) m_look_id = -1;
			for (auto& button : m_buttons)
				if (button.pointer == id) { button.pointer = -1; button.released = true; }
		}

		std::pair<float, float> ConsumeLook() { return std::exchange(m_look_delta, {}); }
		void Reset()
		{
			m_move_id = m_look_id = -1;
			m_buttons = {};
			m_movement = m_look_delta = m_last_look = {};
		}

	private:
		float m_width{ 0.f }, m_height{ 0.f };
		struct ButtonState
		{
			int pointer{ -1 };
			bool released{ false };
		};
		std::array<ButtonState, ButtonCount> m_buttons{};
		int m_move_id{ -1 }, m_look_id{ -1 };
		std::pair<float, float> m_movement{}, m_look_delta{}, m_last_look{};
	};
}
