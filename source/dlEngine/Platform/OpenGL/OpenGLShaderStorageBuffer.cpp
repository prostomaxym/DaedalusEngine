#include "dlpch.h"
#include "OpenGLShaderStorageBuffer.h"

#include "OpenGLHeaders.h"

using namespace Daedalus;

OpenGLShaderStorageBuffer::OpenGLShaderStorageBuffer(uint32_t size, uint32_t binding, Type type)
{
	glGenBuffers(1, &m_rendererID);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_rendererID);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_rendererID);

	if (type == Type::Static)
		glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_STATIC_DRAW);
	else
		glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
}

OpenGLShaderStorageBuffer::~OpenGLShaderStorageBuffer()
{
	glDeleteBuffers(1, &m_rendererID);
}

void OpenGLShaderStorageBuffer::Bind(uint32_t binding) const
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_rendererID);
}

void OpenGLShaderStorageBuffer::SetData(const void* data, uint32_t size, uint32_t offset) const
{
	GLint previous = 0;
	glGetIntegerv(GL_COPY_WRITE_BUFFER_BINDING, &previous);
	glBindBuffer(GL_COPY_WRITE_BUFFER, m_rendererID);
	glBufferSubData(GL_COPY_WRITE_BUFFER, offset, size, data);
	glBindBuffer(GL_COPY_WRITE_BUFFER, previous);
}
