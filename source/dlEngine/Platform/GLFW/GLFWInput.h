#pragma once

#include "Core/Input.h"

#include <map>

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

		std::map<int, int> m_current_state;
		std::map<int, int> m_prev_state;
	};

}