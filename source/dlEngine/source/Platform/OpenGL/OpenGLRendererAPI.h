#pragma once

#include "Daedalus/Renderer/RendererAPI.h"

namespace Daedalus {

	class OpenGLRendererAPI final: public RendererAPI
	{
	public:
		void Init() override;
		std::unique_ptr<ShaderLibrary> LoadShaderLibrary(const std::string& path, bool recompile = false) override;
		void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		void SetClearColor(const glm::vec4& color) override;
		void Clear() override;

		void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
		void DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount) override;

		void SetLineWidth(float width) override;
	};
}
