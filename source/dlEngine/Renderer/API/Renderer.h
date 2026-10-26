#pragma once

#include "RendererAPI.h"

#include <glm/glm.hpp>

#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace Daedalus {
	class Camera;
	class Cubemap;
	class LightSource;
	class Mesh;
	class Model;
	class Shader;
	class VertexArray;
	struct LightSSBO;
	struct RendererDebugInfo;

	class DAEDALUS_API Renderer
	{
	public:
		using DebugInfo = RendererDebugInfo;

	public:
		Renderer();
		~Renderer();
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer(Renderer&&) = delete;
		Renderer& operator=(Renderer&&) = delete;

		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

		static void OnWindowResize(uint32_t width, uint32_t height);
		static std::pair<int, int> GetResolution();
		static float GetAspectRatio();

		static void BeginFrame(const Camera* camera,std::optional<int> number_of_objects = std::nullopt);
		static void FlushPipeline();
		static void Submit(const Model* model, const glm::mat4& transform = glm::mat4(1.0f));

		static void Draw(const Shader* shader, const VertexArray* vertex_array, const glm::mat4& transform = glm::mat4(1.0f));
		static void Draw(const Shader* shader, const Mesh* mesh, const glm::mat4& transform = glm::mat4(1.0f));
		static void Draw(const Shader* shader, const Cubemap* model, const glm::mat4& transform = glm::mat4(1.0f));
		static void DrawUnitQuad();

		static void SetLights(const std::vector<LightSource*>& lights);
		static void AddLight(LightSource* light);
		static void RemoveLight(LightSource* light);

		static DebugInfo* GetDebugInfo() { return s_debug; }

	private:
		static void Init();
		static void Shutdown();
		static void UpdateLightShaderData();
		static void UpdateLightSSBO(const std::vector<LightSSBO>& light_UBOs);
		static void UpdateLightSpaceMatricesSSBO(const std::vector<glm::mat4>& light_proj_view);
		static int CalculateNumberOfShadowMaps();

		static void ComputeDebugInfo();

		static void ComputeTexture(std::string_view shader, uint32_t id_in, uint32_t id_out, int w, int h);
		static void ComputeTextureArray(std::string_view shader, uint32_t id, uint32_t id_out, int w, int h, int layer);
		static void ComputeDepthTexture(std::string_view shader, uint32_t id_in, uint32_t id_out, int w, int h);

		// Defined in Renderer.cpp to keep pass/buffer headers out of this header
		struct Data;
		static Data* GetData();

		std::unique_ptr<RendererAPI> m_api;
		std::unique_ptr<Data> m_data;
		std::unique_ptr<DebugInfo> m_debug;
		static Renderer* s_instance; // Non-owning; Engine owns the renderer.
		static DebugInfo* s_debug;
		static Data* s_data;
	};
}
