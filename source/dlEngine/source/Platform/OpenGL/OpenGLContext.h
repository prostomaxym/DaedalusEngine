#pragma once

#include "Daedalus/Renderer/GraphicsContext.h"

#include <GLFW/glfw3.h>

namespace Daedalus {

	class OpenGLContext : public GraphicsContext
	{
	public:
		OpenGLContext(GLFWwindow* window_handle);

		void Init() override;
		void SwapBuffers() override;

	private:
		GLFWwindow* m_window_handle;
	};

}