#pragma once

#include "RenderPass.h"
#include "Renderer/Primitives/FrameBuffer.h"

namespace Daedalus
{
	// Reads: frame_models, view_frustum, viewport size.
	// Writes: PassTexture::Position, Normal, Ambient, Specular, Albedo, Shininess, Depth
	class DeferredGeometryPass : public RenderPass
	{
	public:
		DeferredGeometryPass() = default;
		void Render(FrameContext& context) override;
		void OnResize(int width, int height) override { CreateGBuffer(width, height); }

		void CreateGBuffer(int width, int height);
		const Framebuffer* GetBuffer() const { return m_gbuffer.get(); }

	private:
		std::unique_ptr<Framebuffer> m_gbuffer;
	};
}
