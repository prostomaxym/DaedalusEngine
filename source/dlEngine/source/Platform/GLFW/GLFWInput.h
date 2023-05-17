#pragma once

#include "Daedalus/Core/Input.h"

namespace Daedalus {

	class GLFWInput : public Input
	{
	protected:
		//Keyboard + mouse input
		virtual bool IsKeyPressedImpl(int keycode) override;
		virtual bool IsMouseButtonPressedImpl(int button) override;
		virtual std::pair<float, float> GetMousePosImpl() override;

		//Gamepad input
		virtual bool IsGpadKeyPresseImpl(int keycode) override;
		virtual std::pair<float, float> GetStickPosImpl(StickType type) override;
		virtual float GetBumperPosImpl(BamperType type) override;

	private:

	};

}