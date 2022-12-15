#pragma once

#include "Daedalus/Renderer/IndexBuffer.h"

namespace Daedalus {

class OpenGLIndexBuffer final : public IndexBuffer
{
public:
	OpenGLIndexBuffer(uint32_t* indices, uint32_t count);
	~OpenGLIndexBuffer();

	OpenGLIndexBuffer(const OpenGLIndexBuffer&) = delete;
	OpenGLIndexBuffer(OpenGLIndexBuffer&&) = delete;
	OpenGLIndexBuffer& operator=(const OpenGLIndexBuffer&) = delete;
	OpenGLIndexBuffer& operator=(OpenGLIndexBuffer&&) = delete;

	void Bind() const;
	void Unbind() const;

	uint32_t GetCount() const { return m_count; }

private:
	uint32_t m_rendererID;
	uint32_t m_count;
};

}
