#include "dlpch.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

#include <glad/glad.h>

using namespace Daedalus;

namespace {

	GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
	{
		switch (type)
		{
		case ShaderDataType::Float:    return GL_FLOAT;
		case ShaderDataType::Float2:   return GL_FLOAT;
		case ShaderDataType::Float3:   return GL_FLOAT;
		case ShaderDataType::Float4:   return GL_FLOAT;
		case ShaderDataType::Mat3:     return GL_FLOAT;
		case ShaderDataType::Mat4:     return GL_FLOAT;
		case ShaderDataType::Int:      return GL_INT;
		case ShaderDataType::Int2:     return GL_INT;
		case ShaderDataType::Int3:     return GL_INT;
		case ShaderDataType::Int4:     return GL_INT;
		case ShaderDataType::Bool:     return GL_BOOL;
		}

		DL_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}
}

OpenGLVertexArray::OpenGLVertexArray() :
	m_rendererID(0),
	m_vertex_buffer_index(0)
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

void OpenGLVertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertex_buffer)
{
	DL_CORE_ASSERT(vertex_buffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");

	glBindVertexArray(m_rendererID);
	vertex_buffer->Bind();

	const auto& layout = vertex_buffer->GetLayout();
	for (const auto& element : layout)
	{
		switch (element.type)
		{
		case ShaderDataType::Float:
		case ShaderDataType::Float2:
		case ShaderDataType::Float3:
		case ShaderDataType::Float4:
		{
			glEnableVertexAttribArray(m_vertex_buffer_index);
			glVertexAttribPointer(m_vertex_buffer_index,
				element.GetComponentCount(),
				ShaderDataTypeToOpenGLBaseType(element.type),
				element.normalized ? GL_TRUE : GL_FALSE,
				layout.GetStride(),
				(const void*)element.offset);
			m_vertex_buffer_index++;
			break;
		}
		case ShaderDataType::Int:
		case ShaderDataType::Int2:
		case ShaderDataType::Int3:
		case ShaderDataType::Int4:
		case ShaderDataType::Bool:
		{
			glEnableVertexAttribArray(m_vertex_buffer_index);
			glVertexAttribIPointer(m_vertex_buffer_index,
				element.GetComponentCount(),
				ShaderDataTypeToOpenGLBaseType(element.type),
				layout.GetStride(),
				(const void*)element.offset);
			m_vertex_buffer_index++;
			break;
		}
		case ShaderDataType::Mat3:
		case ShaderDataType::Mat4:
		{
			uint8_t count = element.GetComponentCount();
			for (uint8_t i = 0; i < count; i++)
			{
				glEnableVertexAttribArray(m_vertex_buffer_index);
				glVertexAttribPointer(m_vertex_buffer_index,
					count,
					ShaderDataTypeToOpenGLBaseType(element.type),
					element.normalized ? GL_TRUE : GL_FALSE,
					layout.GetStride(),
					(const void*)(element.offset + sizeof(float) * count * i));
				glVertexAttribDivisor(m_vertex_buffer_index, 1);
				m_vertex_buffer_index++;
			}
			break;
		}
		default:
			DL_CORE_ASSERT(false, "Unknown ShaderDataType!");
		}
	}

	m_vertex_buffers.push_back(vertex_buffer);
}

void OpenGLVertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& index_buffer)
{
	glBindVertexArray(m_rendererID);

	index_buffer->Bind();

	m_index_buffer = index_buffer;
}