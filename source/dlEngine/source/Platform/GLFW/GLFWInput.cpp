#include "dlpch.h"
#include "GLFWInput.h"

#include "Daedalus/Core/Application.h"

#include <GLFW/glfw3.h>

using namespace Daedalus;

bool GLFWInput::IsKeyPressedImpl(int keycode)
{
	const auto window = static_cast<GLFWwindow*>(Application::GetInstance()->GetWindow().GetNativeWindow());
	const auto state = glfwGetKey(window, keycode);

	return state == GLFW_PRESS || state == GLFW_REPEAT;
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
	GLFWgamepadstate state;
	if (!glfwGetGamepadState(0, &state))
		return false;

	return state.buttons[keycode];
}

std::pair<float, float> Daedalus::GLFWInput::GetStickPosImpl(StickType type)
{
	GLFWgamepadstate state;
	if (!glfwGetGamepadState(0, &state))
		return { 0.f, 0.f };

	switch (type)
	{
	case Input::StickType::Left:
		return { state.axes[GLFW_GAMEPAD_AXIS_LEFT_X], state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] };
	case Input::StickType::Right:
		return { state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] };
	default:
		break;
	}

	return { 0.f, 0.f };
}

float Daedalus::GLFWInput::GetBumperPosImpl(BamperType type)
{
	GLFWgamepadstate state;
	if (!glfwGetGamepadState(0, &state))
		return 0.f;

	switch (type)
	{
	case Input::BamperType::Left:
		return state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
	case Input::BamperType::Right:
		return state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];
	default:
		break;
	}

	return 0.f;
}
