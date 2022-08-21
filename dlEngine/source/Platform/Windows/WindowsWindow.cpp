#include "dlpch.h"
#include "WindowsWindow.h"

namespace Daedalus {

static bool s_GLFWInitialized = false;

WindowsWindow::WindowsWindow(const WindowProps& props)
{
	Init(props);
}

WindowsWindow::~WindowsWindow()
{
	Shutdown();
}

void WindowsWindow::Init(const WindowProps& props)
{
	m_data.title = props.title;
	m_data.width = props.width;
	m_data.height = props.height;

	DL_CORE_INFO("Creating window {0} ({1}, {2})", props.title, props.width, props.height);

	if (!s_GLFWInitialized)
	{
		int success = glfwInit();
		DL_CORE_ASSERT(success, "Could not intialize GLFW!");

		s_GLFWInitialized = true;
	}

	m_window = glfwCreateWindow(props.width, props.height, m_data.title.c_str(), nullptr, nullptr);
	glfwMakeContextCurrent(m_window);
	glfwSetWindowUserPointer(m_window, &m_data);
	SetVSync(false);
}

void WindowsWindow::Shutdown()
{
	glfwDestroyWindow(m_window);
}

void WindowsWindow::OnUpdate()
{
	glfwPollEvents();
	glfwSwapBuffers(m_window);
}

void WindowsWindow::SetVSync(bool enabled)
{
	if (enabled)
		glfwSwapInterval(1);
	else
		glfwSwapInterval(0);

	m_data.VSync = enabled;
}

bool WindowsWindow::IsVSync() const
{
	return m_data.VSync;
}

}