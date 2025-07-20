#pragma once

#include "Renderer/Primitives/FrameBuffer.h"
#include "Renderer/Resourses/Model.h"

namespace Daedalus
{
	class DeferredGeometryPass
	{
	public:
		struct PassIn
		{
			PassIn(std::vector<std::pair<const Model*, glm::mat4>>& models, const Frustum& frustum, int width, int height) :
				frame_models(models),
				view_frustum(frustum),
				viewport_width(width),
				viewport_height(height)
			{ }

			const std::vector<std::pair<const Model*, glm::mat4>>& frame_models;
			Frustum view_frustum;
			int viewport_width{ 0 };
			int viewport_height{ 0 };
		};

		struct PassOut
		{
			uint32_t pos_texture;
			uint32_t norm_texture;
			uint32_t ambient_texture;
			uint32_t spec_texture;
			uint32_t albedo_texture;
			uint32_t shininess_texture;
		};

	public:
		DeferredGeometryPass() = default;
		PassOut Render(const PassIn& data) const;

		void CreateGBuffer(int width, int height);
		const Framebuffer* GetBuffer() const { return m_gbuffer.get(); }

	private:
		std::unique_ptr<Framebuffer> m_gbuffer;
	};
}