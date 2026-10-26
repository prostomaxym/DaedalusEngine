#pragma once

#include "Renderer/Primitives/GraphicsContext.h"

struct GLFWwindow;

namespace Daedalus {

	class OpenGLContext final : public GraphicsContext
	{
	public:
		OpenGLContext(GLFWwindow* window_handle);

		void Init() override;
		void SwapBuffers() override;

	private:
		GLFWwindow* m_window_handle;
	};

}