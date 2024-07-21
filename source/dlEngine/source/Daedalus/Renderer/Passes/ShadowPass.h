#pragma once

#include "Daedalus/Renderer/Primitives/FrameBuffer.h"

#include "Daedalus/Renderer/Resourses/Model.h"

namespace Daedalus
{
	class ShadowPass
	{
	public:
		struct PassIn
		{
			PassIn(std::vector<std::pair<const Model*, glm::mat4>>& models) : frame_models(models) { }

			const std::vector<std::pair<const Model*, glm::mat4>>& frame_models;
		};

		struct PassOut
		{
			uint32_t shadow_map_id{ 0 };
		};

	public:
		ShadowPass() = default;
		PassOut Render(const PassIn& data) const;

		void SetNumberOfShadowMaps(int number);

	private:
		std::unique_ptr<Framebuffer> m_framebuffer;
		int m_number_of_shadowmaps{ 0 };
	};
}