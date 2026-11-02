#pragma once

#include "Renderer/API/RenderCommand.h"
#include <memory>


namespace Daedalus {

	class GraphicsContext
	{
	public:
		static std::unique_ptr<GraphicsContext> Create(void* window_handle);

		virtual ~GraphicsContext() = default;

		virtual void Init() = 0;
		virtual void SwapBuffers() = 0;

		void SetViewport(int xpos, int ypos, int width, int height)
		{
			RenderCommand::SetViewport(xpos, ypos, width, height);
		}
	};
}