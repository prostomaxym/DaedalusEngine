#pragma once

#include "Daedalus/Renderer/Resourses/ShaderLibrary.h"
#include "Daedalus/Renderer/Primitives/VertexArray.h"

#include <glm/vec4.hpp>

#include <memory>

namespace Daedalus {

	class DAEDALUS_API RendererAPI
	{
	public:
		enum class API
		{
			None = 0, OpenGL = 1
		};

		enum ClearMode
		{
			DepthBuffer = 0x00000100,
			ColorBuffer = 0x00004000
		};

	public:
		static std::unique_ptr<RendererAPI> Create();

		virtual ~RendererAPI() = default;

		static API GetAPI() { return s_API; }

		virtual void Init() = 0;
		virtual void SetupGraphicSettings() = 0;
		virtual std::unique_ptr<ShaderLibrary> LoadShaderLibrary(const std::filesystem::path& path, bool recompile = false) = 0;

		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void SetClearColor(const glm::vec4& color) = 0;
		virtual void Clear(uint32_t clear_mode) = 0;

		virtual void DrawIndexed(const VertexArray* vertex_array, uint32_t index_count = 0) = 0;
		virtual void DrawUnindexed(const VertexArray* vertex_array, uint32_t vertex_count) = 0;
		virtual void DrawLines(const VertexArray* vertex_array, uint32_t vertex_count) = 0;

		virtual void SetLineWidth(float width) = 0;

		virtual void UnbindTextureSlot(uint32_t slot_number) = 0;

	private:
		static API s_API;
	};

}
