#include "dlpch.h"
#include "OpenGLUniformBuffer.h"

#include <glad/glad.h>

using namespace Daedalus;

OpenGLUniformBuffer::OpenGLUniformBuffer(uint32_t size, uint32_t binding, Type type)
{
	glCreateBuffers(1, &m_rendererID);
	glBindBuffer(GL_UNIFORM_BUFFER, m_rendererID);
	glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_rendererID);

	if (type == Type::Static)
		glNamedBufferData(m_rendererID, size, nullptr, GL_STATIC_DRAW);
	else
		glNamedBufferData(m_rendererID, size, nullptr, GL_DYNAMIC_DRAW);
}

OpenGLUniformBuffer::~OpenGLUniformBuffer()
{
	glDeleteBuffers(1, &m_rendererID);
}

void OpenGLUniformBuffer::Bind(uint32_t binding) const
{
	glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_rendererID);
}

void OpenGLUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset) const
{
	glNamedBufferSubData(m_rendererID, offset, size, data);
}
