#include "dlpch.h"

#include "Input.h"

using namespace Daedalus;

bool Input::m_keyboard_connected = true;
bool Input::m_gamepad_connected = false;
float Input::m_sticks_deadzone = 0.15f; //TODO: fix this workaround - something is causing drifting

std::pair<float, float> Input::GetStickPos(StickType type)
{
	auto pos = s_instance->GetStickPosImpl(type);

	if (std::abs(pos.first) < m_sticks_deadzone && std::abs(pos.second) < m_sticks_deadzone)
		return { 0.f, 0.f };
	else
		return pos;
}
