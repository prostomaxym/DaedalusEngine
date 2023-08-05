#pragma once

#include "RenderCommand.h"
#include "Daedalus/Renderer/Resourses/Mesh.h"
#include "Daedalus/Renderer/Resourses/Model.h"
#include "Daedalus/Renderer/Resourses/Shader.h"
#include "Daedalus/Renderer/Resourses/ShaderLibrary.h"
#include "Daedalus/Renderer/Objects/Cubemap.h"
#include "Daedalus/Renderer/Objects/LightSSBO.h"
#include "Daedalus/Renderer/Objects/OrthographicCamera.h"
#include "Daedalus/Renderer/Objects/PerspectiveCamera.h"
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

		static void SetupGraphicSettings();
		static void LoadShaderLibrary(const std::filesystem::path& path, bool recompile = false);

		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginScene(const OrthographicCamera& camera);
		static void BeginScene(const PerspectiveCamera& camera);
		static void EndScene();

		static void Submit(const Shader* shader, const VertexArray* vertex_array, const glm::mat4& transform = glm::mat4(1.0f));
		static void Submit(const Shader* shader, const Mesh* mesh, const glm::mat4& transform = glm::mat4(1.0f));
		static void Submit(const Shader* shader, const Model* model, const glm::mat4& transform = glm::mat4(1.0f));
		static void Submit(const Shader* shader, const Cubemap* model, const glm::mat4& transform = glm::mat4(1.0f));

		static void UpdateStaticLightSSBO(const std::vector<LightSSBO>& light_UBOs);
		static void UpdateDynamicLightSSBO(const std::vector<LightSSBO>& light_UBOs);

		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

	public:
		static std::unique_ptr<ShaderLibrary> s_shader_library;

	private:
		static std::shared_ptr<UniformBuffer> s_UBO_scene_data;
		static std::shared_ptr<ShaderStorageBuffer> s_SSBO_static_lighting;
		static std::shared_ptr<ShaderStorageBuffer> s_SSBO_dynamic_lighting;
		static std::shared_ptr<UniformBuffer> s_UBO_graphic_config;	
	};
}
