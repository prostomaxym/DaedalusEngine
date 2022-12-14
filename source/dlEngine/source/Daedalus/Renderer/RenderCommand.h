#pragma once

#include "Daedalus/Renderer/RendererAPI.h"
#include "Daedalus/Renderer/ShaderLibrary.h"

#include <memory>

namespace Daedalus {

class RenderCommand
{
public:
	static void Init();
	static std::unique_ptr<ShaderLibrary> LoadShaderLibrary(const std::string& path, bool recompile = false);
	static void Clear();

	static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
	static void SetClearColor(const glm::vec4& color);
	static void SetLineWidth(float width);

	static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0);
	static void DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount);

private:
	static std::unique_ptr<RendererAPI> s_RendererAPI;
};

}
