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
#include "Daedalus/Renderer/Primitives/FrameBuffer.h"
#include "Daedalus/Renderer/Primitives/ShaderStorageBuffer.h"
#include "Daedalus/Renderer/Primitives/UniformBuffer.h"

#include <glm/glm.hpp>

#include <memory>

namespace Daedalus {

	class DAEDALUS_API Renderer
	{
	public:
		static void Init();
		static void Shutdown();

		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
		static void SetupGraphicSettings();
		static void LoadShaderLibrary(const std::filesystem::path& path, bool recompile = false);
		static ShaderLibrary* GetShaderLibrary() { return s_shader_library.get(); }

		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginScene(const Camera* camera);
		static void EndScene();

		static void Submit(const Shader* shader, const VertexArray* vertex_array, const glm::mat4& transform = glm::mat4(1.0f));
		static void Submit(const Shader* shader, const Mesh* mesh, const glm::mat4& transform = glm::mat4(1.0f));
		static void Submit(const Shader* shader, const Model* model, const glm::mat4& transform = glm::mat4(1.0f));
		static void Submit(const Shader* shader, const Cubemap* model, const glm::mat4& transform = glm::mat4(1.0f));

		static void SubmitForShadowBuffer(const Shader* shader, const Model* model, const glm::mat4& transform = glm::mat4(1.0f));
		static void DrawUnitQuad();

		static std::shared_ptr<Framebuffer> GetGBuffer() { return s_g_framebuffer; }
		static std::shared_ptr<Framebuffer> GetShadowFramebuffer() { return s_framebuffer_shadows; }
		static std::shared_ptr<VertexArray> GetUnitQuad() { return s_unit_quad; }

		static void BindGBufferTextures(const Shader* light_pass_shader, int slot);
		static void BindShadowMap(const Shader* color_pass_shader, int first_slot);

		static void UpdateLightSSBO(const std::vector<LightSSBO>& light_UBOs);
		static void UpdateLightSpaceMatricesSSBO(const std::vector<glm::mat4>& light_proj_view);
		static void UpdateNumberOfShadowMap(int number_of_shadow_map);

	private:
		static std::shared_ptr<VertexArray> CreateUnitQuad();

		static std::unique_ptr<ShaderLibrary> s_shader_library;

		static std::shared_ptr<UniformBuffer> s_UBO_scene_data;
		static std::shared_ptr<UniformBuffer> s_UBO_graphic_config;

		static std::shared_ptr<ShaderStorageBuffer> s_SSBO_light_space_matrices;
		static std::shared_ptr<ShaderStorageBuffer> s_SSBO_lighting;

		static std::shared_ptr<Framebuffer> s_framebuffer_shadows;
		static std::shared_ptr<Framebuffer> s_g_framebuffer;

		static std::shared_ptr<VertexArray> s_unit_quad;

		static Frustum s_view_frustum;
		static glm::mat4 s_light_projection_view;
	};
}
