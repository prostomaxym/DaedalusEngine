#pragma once

#include "RenderCommand.h"
#include "Renderer/Resourses/Mesh.h"
#include "Renderer/Resourses/Model.h"
#include "Renderer/Resourses/Shader.h"
#include "Renderer/Resourses/ShaderLibrary.h"
#include "Renderer/Objects/Cubemap.h"
#include "Renderer/Objects/Geometry.h"
#include "Renderer/Objects/LightSource.h"
#include "Renderer/Objects/Camera.h"
#include "Renderer/Passes/ShadowPass.h"
#include "Renderer/Passes/DeferredGeometryPass.h"
#include "Renderer/Passes/DeferredLightPass.h"
#include "Renderer/Passes/SSAOPass.h"
#include "Renderer/Primitives/FrameBuffer.h"
#include "Renderer/Primitives/ShaderStorageBuffer.h"
#include "Renderer/Primitives/UniformBuffer.h"

#include <glm/glm.hpp>

#include <memory>
#include <map>
#include <optional>

namespace Daedalus {

	class DAEDALUS_API Renderer
	{
	public:
		struct DebugInfo
		{
			ShadowPass::PassOut shadow;
			DeferredGeometryPass::PassOut geom;
			SSAOPass::PassOut ssao;
		};

	public:
		static void Init();
		static void Shutdown();

		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
		static void LoadShaderLibrary(const std::filesystem::path& path, bool recompile = false);
		static ShaderLibrary* GetShaderLibrary() { return s_data->shader_library.get(); }

		static void OnWindowResize(uint32_t width, uint32_t height);
		static std::pair<int, int> GetResolution() { return { s_data->window_width, s_data->window_height }; }
		static float GetAspectRatio() { return static_cast<float>(s_data->window_width) / static_cast<float>(s_data->window_height); }

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

		static const DebugInfo* GetDebugInfo() { return s_debug.get(); }

	private:
		static void UpdateLightShaderData();
		static void UpdateLightSSBO(const std::vector<LightSSBO>& light_UBOs);
		static void UpdateLightSpaceMatricesSSBO(const std::vector<glm::mat4>& light_proj_view);
		static int CalculateNumberOfShadowMaps();

		struct Data
		{
			std::unique_ptr<ShaderLibrary> shader_library{ nullptr };

			std::shared_ptr<UniformBuffer> UBO_scene_data{ nullptr };
			std::shared_ptr<UniformBuffer> UBO_graphic_config{ nullptr };
			Frustum view_frustum;
			glm::mat4 scene_proj;
			glm::mat4 scene_view;

			std::vector<LightSource*> lights;
			std::vector<std::pair<const Model*, glm::mat4>> frame_models;

			ShadowPass shadow_pass;
			DeferredGeometryPass geometry_pass;
			DeferredLightPass light_pass;
			SSAOPass ssao_pass;

			std::shared_ptr<VertexArray> unit_quad{ nullptr };

			int window_width{ 0 };
			int window_height{ 0 };
		};

		static std::unique_ptr<DebugInfo> s_debug;
		static std::unique_ptr<Data> s_data;
	};
}
