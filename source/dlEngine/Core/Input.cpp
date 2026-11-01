#include "dlpch.h"

#include "Input.h"
#include <algorithm>
#include <cmath>

using namespace Daedalus;

Input* Input::s_instance = nullptr;

Input::Input()
{
	DL_ASSERT(!s_instance, Log::Categories::EngineCore, "Only one input system may be active");
	if (s_instance) std::abort();
	s_instance = this;
}

Input::~Input()
{
	s_instance = nullptr;
}

Input* Input::GetInstance()
{
	DL_ASSERT(s_instance, Log::Categories::EngineCore, "Input used outside Engine lifetime");
	if (!s_instance) std::abort();
	return s_instance;
}

std::pair<float, float> Input::GetStickPos(StickType type)
{
	if (!IsGamepadInputEnabled()) return { 0.f, 0.f };
	auto pos = GetInstance()->GetStickPosImpl(type);

	if (std::abs(pos.first) < GetInstance()->m_sticks_deadzone && std::abs(pos.second) < GetInstance()->m_sticks_deadzone)
		return { 0.f, 0.f };
	else
		return pos;
}

void Input::Update()
{
	auto& input = *GetInstance();
	input.UpdateImpl();
	const auto revision = KeybindConfig::GetRevision();
	if (input.m_binding_revision != revision)
	{
		for (const auto& [name, binding] : KeybindConfig::GetActionBinds())
			input.m_action_states.try_emplace(name);
		input.m_binding_revision = revision;
	}
	for (auto& [name, state] : input.m_action_states)
	{
		const auto* binding = KeybindConfig::FindActionBind(name);
		if (!binding) continue;
		const bool rebound = state.binding.keyboard != binding->keyboard || state.binding.gamepad_button != binding->gamepad_button;
		state.previous = state.down;
		state.binding = *binding;
		state.keyboard = binding->keyboard >= 0 && (input.IsKeyPressedImpl(binding->keyboard) || input.IsKeyHoldImpl(binding->keyboard));
		state.gamepad = binding->gamepad_button >= 0 && input.IsGpadKeyPressedImpl(binding->gamepad_button);
		state.down = (input.m_keyboard_connected && state.keyboard) || (input.m_gamepad_connected && state.gamepad);
		// Rebinding establishes a baseline rather than manufacturing an activation/release.
		if (rebound) state.previous = state.down;
	}
}

bool Input::IsActionDown(const std::string& name)
{
	const auto& states = GetInstance()->m_action_states;
	const auto it = states.find(name);
	return it != states.end() && it->second.down;
}

bool Input::IsActionPressed(const std::string& name)
{
	const auto& states = GetInstance()->m_action_states;
	const auto it = states.find(name);
	return it != states.end() && it->second.down && !it->second.previous;
}

bool Input::IsActionHold(const std::string& name)
{
	const auto& states = GetInstance()->m_action_states;
	const auto it = states.find(name);
	return it != states.end() && it->second.down && it->second.previous;
}

bool Input::IsActionReleased(const std::string& name)
{
	const auto& states = GetInstance()->m_action_states;
	const auto it = states.find(name);
	return it != states.end() && !it->second.down && it->second.previous;
}

void Input::RefreshActionGates()
{
	for (auto& [name, state] : m_action_states)
		state.down = (m_keyboard_connected && state.keyboard) || (m_gamepad_connected && state.gamepad);
}

void Input::EnableKeyboardInput(bool enable)
{
	auto& input = *GetInstance();
	input.m_keyboard_connected = enable;
	input.RefreshActionGates();
}

void Input::EnableGamepadInput(bool enable)
{
	auto& input = *GetInstance();
	input.m_gamepad_connected = enable;
	input.RefreshActionGates();
}

void Input::SetSticksDeadZone(float perc)
{
	if (std::isfinite(perc) && perc >= 0.f && perc < 1.f) GetInstance()->m_sticks_deadzone = perc;
}

float Input::ReadGamepadAxis(GamepadAxis axis)
{
	if (!m_gamepad_connected) return 0.f;
	switch (axis)
	{
	case GamepadAxis::LeftX: return GetStickPosImpl(StickType::Left).first;
	case GamepadAxis::LeftY: return GetStickPosImpl(StickType::Left).second;
	case GamepadAxis::RightX: return GetStickPosImpl(StickType::Right).first;
	case GamepadAxis::RightY: return GetStickPosImpl(StickType::Right).second;
	case GamepadAxis::LeftTrigger: return (GetBumperPosImpl(BamperType::Left) + 1.f) * 0.5f;
	case GamepadAxis::RightTrigger: return (GetBumperPosImpl(BamperType::Right) + 1.f) * 0.5f;
	default: return 0.f;
	}
}

float Input::GetAxis(const std::string& name)
{
	const auto* binding = KeybindConfig::FindAxisBind(name);
	if (!binding) return 0.f;
	const float digital = static_cast<float>(IsActionDown(binding->positive_action)) - static_cast<float>(IsActionDown(binding->negative_action));
	float analog = GetInstance()->ReadGamepadAxis(binding->gamepad_axis);
	if (!std::isfinite(analog)) analog = 0.f;
	analog = std::clamp(analog, -1.f, 1.f);
	const float magnitude = std::abs(analog);
	analog = magnitude <= binding->deadzone ? 0.f : std::copysign((magnitude - binding->deadzone) / (1.f - binding->deadzone), analog);
	return std::clamp(digital + analog * binding->scale, -1.f, 1.f);
}
