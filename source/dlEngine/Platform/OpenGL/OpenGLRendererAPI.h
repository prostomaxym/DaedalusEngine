#pragma once

#include "Renderer/API/RendererAPI.h"

namespace Daedalus {

	class OpenGLRendererAPI final : public RendererAPI
	{
	public:
		OpenGLRendererAPI() = default;

		void Init() override;
		void SetupGraphicSettings() override;
		void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		void SetClearColor(const glm::vec4& color) override;
		void Clear(uint32_t clear_mode) override;

		void DrawIndexed(const VertexArray* vertex_array, uint32_t index_count) override;
		void DrawUnindexed(const VertexArray* vertex_array, uint32_t vertex_count) override;
		void DrawLines(const VertexArray* vertex_array, uint32_t vertex_count) override;
		void DrawDebugLines(const VertexArray* vertex_array, VertexBuffer* vertex_buffer, const Shader* shader,
			const void* vertices, uint32_t vertex_count, uint32_t vertex_size, bool depth_test) override;

		void SetLineWidth(float width) override;

		void UnbindTextureSlot(uint32_t slot_number) override;

		int GetMaxTextureSize() override;

	private:
		void ConfigLogging();
	};
}
