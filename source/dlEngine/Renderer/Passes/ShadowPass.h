#pragma once

#include "RenderPass.h"
#include "Config/GraphicsConfig.h"
#include "Renderer/Primitives/FrameBuffer.h"

namespace Daedalus
{
	// Reads: frame_models. Writes: PassTexture::ShadowMap
	class ShadowPass : public RenderPass
	{
	public:
		explicit ShadowPass() {}
		void Render(FrameContext& context) override;

		void SetNumberOfShadowMaps(int number);
		int GetNumberOfShadowMaps() const { return m_number_of_shadowmaps; }

	private:
		std::unique_ptr<Framebuffer> m_framebuffer;
		int m_number_of_shadowmaps{ 0 };
	};
}
