#pragma once

#include "Events/GamepadEvent.h"
#include "Utils/KeyCodes.h"

#include <array>
#include <chrono>

namespace Daedalus
{
	class GamepadButtonEvents
	{
	public:
		using Clock = std::chrono::steady_clock;
		using Buttons = std::array<unsigned char, DL_GAMEPAD_BUTTON_LAST + 1>;

		template<typename Callback>
		void Update(const Buttons& buttons, Clock::time_point now, const Callback& callback)
		{
			for (int button = 0; button <= DL_GAMEPAD_BUTTON_LAST; ++button)
			{
				auto& state = m_buttons[button];
				const bool pressed = buttons[button] != 0;
				if (pressed && !state.pressed)
				{
					state.pressed = true;
					state.next_repeat = now + std::chrono::milliseconds(500);
					GpadPressedEvent event(button, 0);
					callback(event);
				}
				else if (!pressed && state.pressed)
				{
					state = {};
					GpadReleasedEvent event(button);
					callback(event);
				}
				else if (pressed && now >= state.next_repeat)
				{
					// Match keyboard repeat flags without bursting after a slow frame.
					state.next_repeat = now + std::chrono::milliseconds(50);
					GpadPressedEvent event(button, 1);
					callback(event);
				}
			}
		}

	private:
		struct ButtonState
		{
			bool pressed{ false };
			Clock::time_point next_repeat{};
		};

		std::array<ButtonState, DL_GAMEPAD_BUTTON_LAST + 1> m_buttons{};
	};
}
