#pragma once

#include "RenderCommand.h"
#include "Daedalus/Renderer/Resourses/Mesh.h"
#include "Daedalus/Renderer/Resourses/Model.h"
#include "Daedalus/Renderer/Resourses/Shader.h"
#include "Daedalus/Renderer/Resourses/ShaderLibrary.h"
#include "Daedalus/Renderer/Objects/Cubemap.h"
#include "Daedalus/Renderer/Objects/Geometry.h"
#include "Daedalus/Renderer/Objects/LightSource.h"
#include "Daedalus/Renderer/Objects/Camera.h"
#include "Daedalus/Renderer/Passes/ShadowPass.h"
#include "Daedalus/Renderer/Passes/DeferredGeometryPass.h"
#include "Daedalus/Renderer/Passes/DeferredLightPass.h"
#include "Daedalus/Renderer/Passes/SSAOPass.h"
#include "Daedalus/Renderer/Primitives/FrameBuffer.h"
#include "Daedalus/Renderer/Primitives/ShaderStorageBuffer.h"
#include "Daedalus/Renderer/Primitives/UniformBuffer.h"

#include <glm/glm.hpp>

#include <memory>
#include <map>
#include <optional>

namespace Daedalus {

	class DAEDALUS_API Renderer
	{
	public:
		static void Init();
		static void Shutdown();

		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
		static void LoadShaderLibrary(const std::filesystem::path& path, bool recompile = false);
		static ShaderLibrary* GetShaderLibrary() { return s_shader_library.get(); }

		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginFrame(const Camera* camera,std::optional<int> number_of_objects = std::nullopt);
		static void FlushPipeline();
		static void Submit(const Model* model, const glm::mat4& transform = glm::mat4(1.0f));

		static void Draw(const Shader* shader, const VertexArray* vertex_array, const glm::mat4& transform = glm::mat4(1.0f));
		static void Draw(const Shader* shader, const Mesh* mesh, const glm::mat4& transform = glm::mat4(1.0f));
		static void Draw(const Shader* shader, const Cubemap* model, const glm::mat4& transform = glm::mat4(1.0f));
		static void DrawUnitQuad();

		static void SetLights(std::map<uint32_t, LightSource*> lights);
		static void AddLight(uint32_t id, LightSource* light);
		static void RemoveLight(uint32_t id);

	private:
		static void UpdateLightShaderData();
		static void UpdateLightSSBO(const std::vector<LightSSBO>& light_UBOs);
		static void UpdateLightSpaceMatricesSSBO(const std::vector<glm::mat4>& light_proj_view);		

		static std::unique_ptr<ShaderLibrary> s_shader_library;

		static std::shared_ptr<UniformBuffer> s_UBO_scene_data;
		static std::shared_ptr<UniformBuffer> s_UBO_graphic_config;
		static Frustum s_view_frustum;
		static glm::mat4 s_scene_proj;
		static glm::mat4 s_scene_view;

		static std::map<uint32_t, LightSource*> s_lights;
		static std::vector<std::pair<const Model*, glm::mat4>> s_frame_models;

		static ShadowPass s_shadow_pass;
		static DeferredGeometryPass s_geometry_pass;
		static DeferredLightPass s_light_pass;
		static SSAOPass s_ssao_pass;

		static std::shared_ptr<VertexArray> s_unit_quad;

		static int s_window_width;
		static int s_window_height;
	};
}
