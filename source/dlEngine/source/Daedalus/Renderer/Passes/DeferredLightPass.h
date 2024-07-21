#pragma once

#include "Daedalus/Renderer/Primitives/FrameBuffer.h"
#include "Daedalus/Renderer/Resourses/Model.h"
#include "DeferredGeometryPass.h"

namespace Daedalus
{
	class DeferredLightPass
	{
	public:
		struct PassIn
		{
			PassIn(const DeferredGeometryPass::PassOut& geom, uint32_t shadowmap, int w, int h);

			int width{ 0 };
			int height{ 0 };
			uint32_t pos_texture{ 0 };
			uint32_t norm_texture{ 0 };
			uint32_t ambient_texture{ 0 };
			uint32_t spec_texture{ 0 };
			uint32_t albedo_texture{ 0 };
			uint32_t shininess_texture{ 0 };
			uint32_t shadowmap_texture{ 0 };
			uint32_t ssao_texture{ 0 };
		};

	public:
		DeferredLightPass() = default;
		void Render(const PassIn& data) const;
	};
}