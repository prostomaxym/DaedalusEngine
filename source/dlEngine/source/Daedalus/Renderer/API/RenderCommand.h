#pragma once

#include "Daedalus/Renderer/API/RendererAPI.h"
#include "Daedalus/Renderer/Resourses/ShaderLibrary.h"

#include <memory>

namespace Daedalus {

	class DAEDALUS_API RenderCommand
	{
	public:
		static void Init();
		static std::unique_ptr<ShaderLibrary> LoadShaderLibrary(const std::filesystem::path& path, bool recompile = false);
		static void Clear();

		static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
		static void SetClearColor(const glm::vec4& color);
		static void SetLineWidth(float width);

		static void DrawIndexed(const VertexArray* vertexArray, uint32_t indexCount = 0);
		static void DrawUnindexed(const VertexArray* vertexArray, uint32_t vertex_count);
		static void DrawLines(const VertexArray* vertexArray, uint32_t vertexCount);

		static void UnbindTextureSlot(uint32_t slot_num);

	private:
		static std::unique_ptr<RendererAPI> s_RendererAPI;
	};

}
