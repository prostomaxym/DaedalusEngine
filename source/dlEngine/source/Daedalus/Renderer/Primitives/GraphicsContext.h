#pragma once

#include "Daedalus/Renderer/API/RenderCommand.h"
#include <GLFW/glfw3.h>

namespace Daedalus {

	class GraphicsContext
	{
	public:
		static std::unique_ptr<GraphicsContext> Create(GLFWwindow* window_handle);

		virtual void Init() = 0;
		virtual void SwapBuffers() = 0;

		void SetViewport(int xpos, int ypos, int width, int height)
		{
			RenderCommand::SetViewport(xpos, ypos, width, height);
		}
	};

}