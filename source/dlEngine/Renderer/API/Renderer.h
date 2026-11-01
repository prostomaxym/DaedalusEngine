#pragma once

#include "RendererAPI.h"
#include "Debug/Profiler.h"

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

		// Graphics thread, one frame only; return false without DL_DEV_TOOLS. Colors are opaque RGB in [0, 1].
		[[nodiscard]] static bool DebugLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, bool depth_test = true);
		[[nodiscard]] static bool DebugBox(const glm::vec3& minimum, const glm::vec3& maximum, const glm::vec3& color, bool depth_test = true);
		[[nodiscard]] static bool DebugSphere(const glm::vec3& center, float radius, const glm::vec3& color, bool depth_test = true, size_t segments = 32);
		static void BeginDebugDrawFrame();
		static void FlushDebugDraw();
		[[nodiscard]] static size_t GetRejectedDebugPrimitives();

		// GPU profiling (dev builds; no-ops otherwise). Graphics thread only.
		// One GPU frame spans BeginGpuProfileFrame..EndGpuProfileFrame; scopes nest inside it.
		static void BeginGpuProfileFrame();
		static void EndGpuProfileFrame();
		static void BeginGpuScope(const char* name); // name must have static storage
		static void EndGpuScope();

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

	class GpuProfileScope
	{
	public:
		explicit GpuProfileScope(const char* name) { Renderer::BeginGpuScope(name); }
		~GpuProfileScope() { Renderer::EndGpuScope(); }

		GpuProfileScope(const GpuProfileScope&) = delete;
		GpuProfileScope& operator=(const GpuProfileScope&) = delete;
		GpuProfileScope(GpuProfileScope&&) = delete;
		GpuProfileScope& operator=(GpuProfileScope&&) = delete;
	};
}

#ifdef DL_DEV_TOOLS
	#define DL_PROFILE_GPU_SCOPE(name) ::Daedalus::GpuProfileScope DL_PROFILE_CONCAT(dl_gpu_scope_, __LINE__)(name)
#else
	#define DL_PROFILE_GPU_SCOPE(name)
#endif
