#include "OpenGLContext.h"

#include <glad/glad.h>

Daedalus::OpenGLContext::OpenGLContext(GLFWwindow* window_handle) :
	m_window_handle(window_handle)
{
}

void Daedalus::OpenGLContext::Init()
{
	glfwMakeContextCurrent(m_window_handle);
	const auto status = gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
	DL_ASSERT(status, Log::Categories::Renderer, "Failed to initialize GLAD");
}

void Daedalus::OpenGLContext::SwapBuffers()
{
	glfwSwapBuffers(m_window_handle);
}
