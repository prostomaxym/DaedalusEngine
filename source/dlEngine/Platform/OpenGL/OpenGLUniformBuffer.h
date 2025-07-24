#pragma once

#include "Renderer/Primitives/UniformBuffer.h"

namespace Daedalus {

	class OpenGLUniformBuffer final : public UniformBuffer
	{
	public:
		OpenGLUniformBuffer(uint32_t size, uint32_t binding, Type type, const void* data);
		~OpenGLUniformBuffer();

		void SetData(const void* data, uint32_t size, uint32_t offset) const override;
		void Bind(uint32_t binding) const override;

	private:
		uint32_t m_rendererID = 0;
	};
}
