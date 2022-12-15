#pragma once

#include "Daedalus/Renderer/VertexArray.h"

namespace Daedalus {

	class OpenGLVertexArray final : public VertexArray
	{
	public:
		OpenGLVertexArray();
		~OpenGLVertexArray();

		OpenGLVertexArray(const OpenGLVertexArray&) = delete;
		OpenGLVertexArray(OpenGLVertexArray&&) = delete;
		OpenGLVertexArray& operator=(const OpenGLVertexArray&) = delete;
		OpenGLVertexArray& operator=(OpenGLVertexArray&&) = delete;

		void Bind() const override;
		void Unbind() const override;

		void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertex_buffer) override;
		void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& index_buffer) override;

		const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const { return m_vertex_buffers; }
		const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const { return m_index_buffer; }

	private:
		uint32_t m_rendererID;
		uint32_t m_vertex_buffer_index;
		std::vector<std::shared_ptr<VertexBuffer>> m_vertex_buffers;
		std::shared_ptr<IndexBuffer> m_index_buffer;
	};

}
