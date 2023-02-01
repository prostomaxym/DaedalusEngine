#pragma once

#include "RenderCommand.h"
#include "Daedalus/Renderer/Resourses/Mesh.h"
#include "Daedalus/Renderer/Resourses/Model.h"
#include "Daedalus/Renderer/Resourses/Shader.h"
#include "Daedalus/Renderer/Resourses/ShaderLibrary.h"
#include "Daedalus/Renderer/Objects/OrthographicCamera.h"
#include "Daedalus/Renderer/Objects/PerspectiveCamera.h"

#include <glm/glm.hpp>

#include <memory>

namespace Daedalus {

class DAEDALUS_API Renderer
{
public:
	static void Init();
	static void Shutdown();
	static void LoadShaderLibrary(const std::filesystem::path& path, bool recompile = false);

	static void OnWindowResize(uint32_t width, uint32_t height);

	static void BeginScene(const OrthographicCamera& camera);
	static void BeginScene(const PerspectiveCamera& camera);
	static void EndScene();

	static void Submit(const Shader* shader, const VertexArray* vertex_array, const glm::mat4& transform = glm::mat4(1.0f));
	static void Submit(const Shader* shader, const Mesh* mesh);
	static void Submit(const Shader* shader, const Model* model);

	static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

public:
	static std::unique_ptr<ShaderLibrary> s_shader_library;

private:
	struct SceneData
	{
		glm::mat4 ProjectionViewMatrix;
	};
	static std::unique_ptr<SceneData> s_scene_data;
};

}
