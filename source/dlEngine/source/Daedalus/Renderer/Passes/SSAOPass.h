#pragma once

#include "Daedalus/Renderer/Primitives/FrameBuffer.h"
#include "Daedalus/Renderer/Resourses/Model.h"
#include "Daedalus/Renderer/Buffers/SSAO.h"

namespace Daedalus
{
	class SSAOPass
	{
	public:
		struct PassIn
		{
			PassIn(uint32_t pos, uint32_t norm, glm::mat4 p, glm::mat4 v) :
				position_texture(pos),
				normal_texture(norm),
				proj(p),
				view(v)
				{}

			uint32_t position_texture{ 0 };
			uint32_t normal_texture{ 0 };
			glm::mat4 proj;
			glm::mat4 view;
		};

		struct PassOut
		{
			uint32_t ssao_texture{ 0 };
		};

	public:
		SSAOPass() = default;
		PassOut Render(const PassIn& data) const;

		void CreateSSAOBuffers(int width, int height);
	private:
		std::unique_ptr<SSAO> m_ssao;
	};
}