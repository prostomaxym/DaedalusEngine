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

bool Daedalus::GLFWInput::IsGpadKeyPresseImpl(int keycode)
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
		return { state.axes[0], state.axes[1] };
	case Input::StickType::Right:
		return { state.axes[2], state.axes[3] };
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
		return state.axes[4];
	case Input::BamperType::Right:
		return state.axes[5];
	default:
		break;
	}

	return 0.f;
}
