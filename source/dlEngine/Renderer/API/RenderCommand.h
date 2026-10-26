#pragma once

#include "Renderer/API/RendererAPI.h"
#include <thread>

namespace Daedalus {
	class RendererAPI;
	class VertexArray;

	class DAEDALUS_API RenderCommand
	{
	public:
		static void Init();
		static void SetupGraphicSettings();
		static void Clear(uint32_t clear_mode);

		static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
		static void SetClearColor(const glm::vec4& color);
		static void SetLineWidth(float width);

		static void DrawIndexed(const VertexArray* vertexArray, uint32_t indexCount = 0);
		static void DrawUnindexed(const VertexArray* vertexArray, uint32_t vertex_count);
		static void DrawLines(const VertexArray* vertexArray, uint32_t vertexCount);

		static void UnbindTextureSlot(uint32_t slot_num);

		static int GetMaxTextureSize();

	private:
		friend class Renderer;
		static void Bind(RendererAPI* api);
		static RendererAPI& GetAPI();
		static RendererAPI* s_renderer_api; // Non-owning; Renderer owns the backend.
		static std::thread::id s_owner_thread;
	};

}
