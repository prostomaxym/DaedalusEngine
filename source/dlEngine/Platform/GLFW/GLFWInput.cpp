#include "dlpch.h"
#include "GLFWInput.h"

#include "Core/Application.h"
#include "Core/Window.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <iterator>

using namespace Daedalus;

GLFWInput::GLFWInput()
{
	m_gamepad_axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] = -1.f;
	m_gamepad_axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] = -1.f;
}

void GLFWInput::UpdateImpl()
{
	const auto window = static_cast<GLFWwindow*>(Application::GetInstance()->GetWindow().GetNativeWindow());

	for (int i = DL_KEY_SPACE; i <= DL_KEY_MENU; ++i)
	{
		m_prev_state[i] = m_current_state[i];
		if (KeybindConfig::IsValidKeyboardCode(i)) m_current_state[i] = static_cast<unsigned char>(glfwGetKey(window, i));
	}
	GLFWgamepadstate state{};
	if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state))
	{
		std::copy(std::begin(state.buttons), std::end(state.buttons), m_gamepad_buttons.begin());
		std::copy(std::begin(state.axes), std::end(state.axes), m_gamepad_axes.begin());
	}
	else
	{
		m_gamepad_buttons.fill(GLFW_RELEASE);
		m_gamepad_axes.fill(0.f);
		m_gamepad_axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] = -1.f;
		m_gamepad_axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] = -1.f;
	}
}

bool GLFWInput::IsKeyPressedImpl(int keycode)
{
	return keycode >= 0 && keycode <= DL_KEY_MENU && m_current_state[keycode] == GLFW_PRESS && m_prev_state[keycode] == GLFW_RELEASE;
}

bool GLFWInput::IsKeyHoldImpl(int keycode)
{
	return keycode >= 0 && keycode <= DL_KEY_MENU && m_current_state[keycode] == GLFW_PRESS && m_prev_state[keycode] == GLFW_PRESS;
}

bool GLFWInput::IsKeyReleasedImpl(int keycode)
{
	return keycode >= 0 && keycode <= DL_KEY_MENU && m_current_state[keycode] == GLFW_RELEASE && m_prev_state[keycode] == GLFW_PRESS;
}

bool GLFWInput::IsMouseButtonPressedImpl(int button)
{
	const auto window = static_cast<GLFWwindow*>(Application::GetInstance()->GetWindow().GetNativeWindow());
	const auto state = glfwGetMouseButton(window, button);

	return state == GLFW_PRESS;
}

std::pair<float, float> GLFWInput::GetMousePosImpl()
{
	const auto window = static_cast<GLFWwindow*>(Application::GetInstance()->GetWindow().GetNativeWindow());
	double xpos = 0.0, ypos = 0.0;
	glfwGetCursorPos(window, &xpos, &ypos);

	return std::make_pair<float, float>(static_cast<float>(xpos), static_cast<float>(ypos));
}

bool Daedalus::GLFWInput::IsGpadKeyPressedImpl(int keycode)
{
	return keycode >= 0 && keycode <= DL_GAMEPAD_BUTTON_LAST && m_gamepad_buttons[keycode] == GLFW_PRESS;
}

std::pair<float, float> Daedalus::GLFWInput::GetStickPosImpl(StickType type)
{
	switch (type)
	{
	case Input::StickType::Left:
		return { m_gamepad_axes[GLFW_GAMEPAD_AXIS_LEFT_X], m_gamepad_axes[GLFW_GAMEPAD_AXIS_LEFT_Y] };
	case Input::StickType::Right:
		return { m_gamepad_axes[GLFW_GAMEPAD_AXIS_RIGHT_X], m_gamepad_axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] };
	default:
		break;
	}

	return { 0.f, 0.f };
}

float Daedalus::GLFWInput::GetBumperPosImpl(BamperType type)
{
	switch (type)
	{
	case Input::BamperType::Left:
		return m_gamepad_axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
	case Input::BamperType::Right:
		return m_gamepad_axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];
	default:
		break;
	}

	return 0.f;
}
