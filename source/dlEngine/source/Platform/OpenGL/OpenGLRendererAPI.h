#pragma once

#include "Daedalus/Renderer/API/RendererAPI.h"
#include "Daedalus/Renderer/Resourses/Texture.h"

namespace Daedalus {

	class OpenGLRendererAPI final : public RendererAPI
	{
	public:
		OpenGLRendererAPI();

		void Init() override;
		void SetupGraphicSettings() override;
		void SetupShaderSettings() override;
		std::unique_ptr<ShaderLibrary> LoadShaderLibrary(const std::filesystem::path& path, bool recompile = false) override;
		void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		void SetClearColor(const glm::vec4& color) override;
		void Clear() override;

		void DrawIndexed(const VertexArray* vertex_array, uint32_t index_count = 0) override;
		void DrawUnindexed(const VertexArray* vertex_array, uint32_t vertex_count) override;
		void DrawLines(const VertexArray* vertex_array, uint32_t vertex_count) override;

		void SetLineWidth(float width) override;

		void UnbindTextureSlot(uint32_t slot_number) override;

	private:
		void ConfigLogging();
	};
}
