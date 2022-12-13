#pragma once

#include "VertexArray.h"

#include <glm/vec4.hpp>

#include <memory>

namespace Daedalus {

	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0, OpenGL = 1
		};
	public:
		static std::unique_ptr<RendererAPI> Create();

		virtual ~RendererAPI() = default;

		static API GetAPI() { return s_API; }

		virtual void Init() = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void SetClearColor(const glm::vec4& color) = 0;
		virtual void Clear() = 0;

		virtual void DrawIndexed(const std::shared_ptr<VertexArray>& vertex_array, uint32_t index_count = 0) = 0;
		virtual void DrawLines(const std::shared_ptr<VertexArray>& vertex_array, uint32_t vertex_count) = 0;

		virtual void SetLineWidth(float width) = 0;

	private:
		static API s_API;
	};

}
