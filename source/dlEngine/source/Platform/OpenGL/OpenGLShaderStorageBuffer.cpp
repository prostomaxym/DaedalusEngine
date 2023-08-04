#include "dlpch.h"
#include "OpenGLShaderStorageBuffer.h"

#include <glad/glad.h>

using namespace Daedalus;

OpenGLShaderStorageBuffer::OpenGLShaderStorageBuffer(uint32_t size, uint32_t binding, Type type)
{
	glCreateBuffers(1, &m_rendererID);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_rendererID);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_rendererID);

	if (type == Type::Static)
		glNamedBufferData(m_rendererID, size, nullptr, GL_STATIC_DRAW);
	else
		glNamedBufferData(m_rendererID, size, nullptr, GL_DYNAMIC_DRAW);
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
	glNamedBufferSubData(m_rendererID, offset, size, data);
}
