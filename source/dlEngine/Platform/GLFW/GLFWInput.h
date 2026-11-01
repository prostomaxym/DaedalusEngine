#pragma once

#include "Core/Input.h"

#include <array>

namespace Daedalus {

	class GLFWInput : public Input
	{
	public:
		GLFWInput();

		void UpdateImpl() override;

	protected:
		//Keyboard + mouse input
		virtual bool IsKeyPressedImpl(int keycode) override;
		virtual bool IsKeyHoldImpl(int keycode) override;
		virtual bool IsKeyReleasedImpl(int keycode) override;
		virtual bool IsMouseButtonPressedImpl(int button) override;
		virtual std::pair<float, float> GetMousePosImpl() override;

		//Gamepad input
		virtual bool IsGpadKeyPressedImpl(int keycode) override;
		virtual std::pair<float, float> GetStickPosImpl(StickType type) override;
		virtual float GetBumperPosImpl(BamperType type) override;

		std::array<unsigned char, DL_KEY_MENU + 1> m_current_state{};
		std::array<unsigned char, DL_KEY_MENU + 1> m_prev_state{};
		std::array<unsigned char, DL_GAMEPAD_BUTTON_LAST + 1> m_gamepad_buttons{};
		std::array<float, DL_GAMEPAD_AXIS_LAST + 1> m_gamepad_axes{};
	};

}