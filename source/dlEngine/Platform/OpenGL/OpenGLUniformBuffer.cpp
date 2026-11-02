#include "dlpch.h"
#include "OpenGLUniformBuffer.h"

#include "OpenGLHeaders.h"

using namespace Daedalus;

OpenGLUniformBuffer::OpenGLUniformBuffer(uint32_t size, uint32_t binding, Type type, const void* data)
{
	glGenBuffers(1, &m_rendererID);
	glBindBuffer(GL_UNIFORM_BUFFER, m_rendererID);
	glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_rendererID);

	if (type == Type::Static)
		glBufferData(GL_UNIFORM_BUFFER, size, data, GL_STATIC_DRAW);
	else
		glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
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
	GLint previous = 0;
	glGetIntegerv(GL_COPY_WRITE_BUFFER_BINDING, &previous);
	glBindBuffer(GL_COPY_WRITE_BUFFER, m_rendererID);
	glBufferSubData(GL_COPY_WRITE_BUFFER, offset, size, data);
	glBindBuffer(GL_COPY_WRITE_BUFFER, previous);
}
