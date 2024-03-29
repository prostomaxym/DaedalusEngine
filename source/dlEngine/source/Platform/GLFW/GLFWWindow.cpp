#include "dlpch.h"
#include "GLFWWindow.h"

#include "Daedalus/Core/Input.h"
#include "Daedalus/Events/ApplicationEvent.h"
#include "Daedalus/Events/MouseEvent.h"
#include "Daedalus/Events/KeyEvent.h"
#include "Daedalus/Renderer/Primitives/GraphicsContext.h"

using namespace Daedalus;

static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error_code, const char* message)
{
	Log::Write(Log::Levels::Error, Log::Categories::Platform, "GLFW Error ({0}): {1}", error_code, message);
}

GLFWWindow::GLFWWindow(const WindowProps& props)
{
	Init(props);
	Log::Write(Log::Levels::Info, Log::Categories::Platform, "Window is created;");
}

GLFWWindow::~GLFWWindow()
{
	Shutdown();
}

void GLFWWindow::Init(const WindowProps& props)
{
	m_data.title = props.title;
	m_data.width = props.width;
	m_data.height = props.height;
	m_screenmode = props.screenmode;

	if (!s_GLFWInitialized)
	{
		const auto success = glfwInit();
		DL_ASSERT(success, Log::Categories::Platform, "Could not intialize GLFW!");
		glfwSetErrorCallback(GLFWErrorCallback);

		s_GLFWInitialized = true;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_TRUE);
	
	if (props.screenmode != WindowProps::ScreenMode::windowed)
	{
		const auto monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		Log::Write(Log::Levels::Info, Log::Categories::Platform, "Creating window {0} ({1}, {2})", props.title, mode->width, mode->height);

		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

		if (props.screenmode == WindowProps::ScreenMode::fullscreen)
			m_window = glfwCreateWindow(mode->width, mode->height, m_data.title.c_str(), monitor, nullptr);
		else
			m_window = glfwCreateWindow(mode->width, mode->height, m_data.title.c_str(), nullptr, nullptr);		
	}
	else
	{
		glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

		Log::Write(Log::Levels::Info, Log::Categories::Platform, "Creating window {0} ({1}, {2})", props.title, props.width, props.height);
		m_window = glfwCreateWindow(props.width, props.height, m_data.title.c_str(), nullptr, nullptr);
	}

	m_context = GraphicsContext::Create(m_window);
	m_context->Init();

	glfwSetWindowUserPointer(m_window, &m_data);
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (glfwJoystickPresent(0))
		Input::EnableGamepadInput(true);

	SetVSync(props.vsync);

	SetupCallbacks();
}

void GLFWWindow::Shutdown()
{
	glfwDestroyWindow(m_window);
}

void GLFWWindow::SetupCallbacks()
{
	glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
			data.width = width;
			data.height = height;

			WindowResizeEvent event(width, height);
			data.EventCallback(event);
		});

	glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window)
		{
			WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
			WindowCloseEvent event;
			data.EventCallback(event);
		});

	glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

			switch (action)
			{
			case GLFW_PRESS:
			{
				KeyPressedEvent event(key, 0);
				data.EventCallback(event);
				break;
			}
			case GLFW_RELEASE:
			{
				KeyReleasedEvent event(key);
				data.EventCallback(event);
				break;
			}
			case GLFW_REPEAT:
			{
				KeyPressedEvent event(key, 1);
				data.EventCallback(event);
				break;
			}
			}
		});

	glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int keycode)
		{
			WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
			KeyTypedEvent event(keycode);
			data.EventCallback(event);
		});

	glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods)
		{
			WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

			switch (action)
			{
			case GLFW_PRESS:
			{
				MouseButtonPressedEvent event(button);
				data.EventCallback(event);
				break;
			}
			case GLFW_RELEASE:
			{
				MouseButtonReleasedEvent event(button);
				data.EventCallback(event);
				break;
			}
			}
		});

	glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

			MouseScrolledEvent event(static_cast<float>(xOffset), static_cast<float>(yOffset));
			data.EventCallback(event);
		});

	glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xPos, double yPos)
		{
			WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
			MouseMovedEvent event(static_cast<float>(xPos), static_cast<float>(yPos));
			data.EventCallback(event);
		});

	glfwSetWindowFocusCallback(m_window, [](GLFWwindow* window, int focused)
		{
			WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
			data.focused = focused;

			// If the window is not focused, disable VSync to avoid potential freezing issues
			data.VSync = !focused ? false : data.VSync;

			if (data.VSync)
				glfwSwapInterval(1);
			else
				glfwSwapInterval(0);
		});

	glfwSetJoystickCallback([](int joystick_id, int event)
		{
			Input::EnableGamepadInput(event == GLFW_CONNECTED);
		});
}

void GLFWWindow::OnUpdate()
{
	glfwPollEvents();
	m_context->SwapBuffers();
}

void GLFWWindow::SetVSync(bool enabled)
{
	if (enabled)
		glfwSwapInterval(1);
	else
		glfwSwapInterval(0);

	m_data.VSync = enabled;
}

bool GLFWWindow::IsVSync() const
{
	return m_data.VSync;
}

bool GLFWWindow::IsFullscreen() const
{
	return m_screenmode != WindowProps::ScreenMode::windowed;
}

void GLFWWindow::SetWindowMode(WindowProps::ScreenMode screenmode)
{
	if (screenmode == m_screenmode)
		return;

	m_screenmode = screenmode;

	static int pos_x = 0, pos_y = 0, width = 0, height = 0;

	if (m_screenmode != WindowProps::ScreenMode::windowed)
	{
		const auto monitor = glfwGetPrimaryMonitor();

		// backup window position and window size
		glfwGetWindowPos(m_window, &pos_x, &pos_y);
		glfwGetWindowSize(m_window, &width, &height);

		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		if(m_screenmode == WindowProps::ScreenMode::fullscreen)
			glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
		else
			glfwSetWindowMonitor(m_window, nullptr, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);

		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		m_context->SetViewport(0, 0, mode->width, mode->height);
		
		m_data.width = mode->width;
		m_data.height = mode->height;
	}
	else
	{
		glfwSetWindowMonitor(m_window, nullptr, pos_x, pos_y, width, height, GLFW_DONT_CARE);
		glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
		m_context->SetViewport(pos_x, pos_y, width, height);

		m_data.width = width;
		m_data.height = height;
	}

	SetVSync(m_data.VSync);
	glfwMakeContextCurrent(m_window);
}