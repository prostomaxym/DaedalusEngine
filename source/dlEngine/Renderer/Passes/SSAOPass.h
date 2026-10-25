#pragma once

#include "RenderPass.h"
#include "Config/GraphicsConfig.h"
#include "Renderer/Primitives/FrameBuffer.h"
#include "Renderer/Buffers/SSAO.h"

namespace Daedalus
{
	// Reads: PassTexture::Position, Normal, scene_proj, scene_view. Writes: PassTexture::SSAO. Skipped when SSAO is disabled
	class SSAOPass : public RenderPass
	{
	public:
		explicit SSAOPass() {}
		void Render(FrameContext& context) override;
		void OnResize(int width, int height) override { CreateSSAOBuffers(width, height); }

		void CreateSSAOBuffers(int width, int height);

	private:
		std::unique_ptr<SSAO> m_ssao;
	};
}
