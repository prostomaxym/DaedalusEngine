#include "dlpch.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

#include <glad/glad.h>

using namespace Daedalus;

OpenGLVertexArray::OpenGLVertexArray()
{
	glCreateVertexArrays(1, &m_rendererID);
}

OpenGLVertexArray::~OpenGLVertexArray()
{
	glDeleteVertexArrays(1, &m_rendererID);
}

void OpenGLVertexArray::Bind() const
{
	glBindVertexArray(m_rendererID);
}

void OpenGLVertexArray::Unbind() const
{
	glBindVertexArray(0);
}