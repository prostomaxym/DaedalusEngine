#pragma once

#include "Daedalus/Renderer/IndexBuffer.h"

namespace Daedalus {

class OpenGLIndexBuffer : public IndexBuffer
{
public:
	OpenGLIndexBuffer(uint32_t* indices, uint32_t count);
	virtual ~OpenGLIndexBuffer();

	OpenGLIndexBuffer(const OpenGLIndexBuffer&) = delete;
	OpenGLIndexBuffer(OpenGLIndexBuffer&&) = delete;
	OpenGLIndexBuffer& operator=(const OpenGLIndexBuffer&) = delete;
	OpenGLIndexBuffer& operator=(OpenGLIndexBuffer&&) = delete;

	virtual void Bind() const;
	virtual void Unbind() const;

	virtual uint32_t GetCount() const { return m_count; }
private:
	uint32_t m_rendererID;
	uint32_t m_count;
};

}
