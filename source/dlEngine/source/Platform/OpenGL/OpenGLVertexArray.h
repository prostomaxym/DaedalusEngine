#pragma once

#include "Daedalus/Renderer/VertexArray.h"

namespace Daedalus {

	class OpenGLVertexArray : public VertexArray
	{
	public:
		OpenGLVertexArray();
		virtual ~OpenGLVertexArray();

		virtual void Bind() const override;
		virtual void Unbind() const override;

	private:
		uint32_t m_rendererID;
		uint32_t m_vertex_buffer_index = 0;
	};

}
