#pragma once

#include "Daedalus/Renderer/Primitives/ShaderStorageBuffer.h"

namespace Daedalus {

	class OpenGLShaderStorageBuffer final : public ShaderStorageBuffer
	{
	public:
		OpenGLShaderStorageBuffer(uint32_t size, uint32_t binding, Type type);
		~OpenGLShaderStorageBuffer();

		void SetData(const void* data, uint32_t size, uint32_t offset = 0) const override;
		void Bind(uint32_t binding) const override;

	private:
		uint32_t m_rendererID = 0;
	};
}
