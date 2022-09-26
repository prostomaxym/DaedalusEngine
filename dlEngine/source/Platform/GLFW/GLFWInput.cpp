#include "dlpch.h"
#include "GLFWInput.h"

#include "Daedalus/Core/Application.h"

#include <GLFW/glfw3.h>

using namespace Daedalus;

bool GLFWInput::IsKeyPressedImpl(int keycode)
{
	const auto& window = static_cast<GLFWwindow*>(Application::GetInstance()->GetWindow().GetNativeWindow());
	const auto state = glfwGetKey(window, keycode);

	return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool GLFWInput::IsMouseButtonPressedImpl(int button)
{
	const auto& window = static_cast<GLFWwindow*>(Application::GetInstance()->GetWindow().GetNativeWindow());
	const auto state = glfwGetMouseButton(window, button);

	return state == GLFW_PRESS;
}

std::pair<float, float> GLFWInput::GetMousePosImpl()
{
	const auto& window = static_cast<GLFWwindow*>(Application::GetInstance()->GetWindow().GetNativeWindow());
	double xpos = 0.0, ypos = 0.0;
	glfwGetCursorPos(window, &xpos, &ypos);

	return std::make_pair<float, float>(static_cast<float>(xpos), static_cast<float>(ypos));
}
