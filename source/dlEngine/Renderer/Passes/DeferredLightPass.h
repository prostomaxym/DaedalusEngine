#pragma once

#include "RenderPass.h"

namespace Daedalus
{
	// Reads: viewport size, G-buffer textures, ShadowMap, SSAO. Draws the final image to the default framebuffer
	class DeferredLightPass : public RenderPass
	{
	public:
		explicit DeferredLightPass() {}
		void Render(FrameContext& context) override;
	};
}
