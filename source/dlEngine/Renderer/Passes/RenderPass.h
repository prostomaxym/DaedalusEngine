#pragma once

#include "FrameContext.h"

namespace Daedalus
{
	class RenderPass
	{
	public:
		virtual ~RenderPass() = default;

		virtual void Render(FrameContext& context) = 0;

		// Recreate size dependent resources
		virtual void OnResize(int width, int height) {}
	};
}
