#pragma once

#include "Daedalus/Core/Core.h"

#include <tuple>

namespace Daedalus {

class DAEDALUS_API Input
{
public:
	inline static bool IsKeyPressed(int keycode) { return s_instance->IsKeyPressedImpl(keycode); }

	inline static bool IsMouseButtonPressed(int button) { return s_instance->IsMouseButtonPressedImpl(button); }
	inline static std::pair<float,float> GetMousePos() { return s_instance->GetMousePosImpl(); }

protected:
	virtual bool IsKeyPressedImpl(int keycode) = 0;

	virtual bool IsMouseButtonPressedImpl(int button) = 0;
	virtual std::pair<float, float> GetMousePosImpl() = 0;

private:
	static Input* s_instance;
	bool s_has_mouse_keyboard = false;
	bool s_has_gamepad = false;
};

}