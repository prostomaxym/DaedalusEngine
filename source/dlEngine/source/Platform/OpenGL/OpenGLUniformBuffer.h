#pragma once

#include "Daedalus/Renderer/Primitives/UniformBuffer.h"

namespace Daedalus {

	class OpenGLUniformBuffer final : public UniformBuffer
	{
	public:
		OpenGLUniformBuffer(uint32_t size, uint32_t binding, Type type);
		~OpenGLUniformBuffer();

		void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;

	private:
		uint32_t m_rendererID = 0;
	};
}
