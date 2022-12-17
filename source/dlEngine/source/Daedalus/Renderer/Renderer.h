#pragma once

#include "OrthographicCamera.h"
#include "RenderCommand.h"
#include "Shader.h"
#include "ShaderLibrary.h"

#include <glm/glm.hpp>

#include <memory>

namespace Daedalus {

class Renderer
{
public:
	static void Init();
	static void Shutdown();
	static void LoadShaderLibrary(const std::string& path, bool recompile = false);

	static void OnWindowResize(uint32_t width, uint32_t height);

	static void BeginScene(OrthographicCamera& camera);
	static void EndScene();

	static void Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertex_array, const glm::mat4& transform = glm::mat4(1.0f));

	static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

public:
	static std::unique_ptr<ShaderLibrary> s_shader_library;

private:
	struct SceneData
	{
		glm::mat4 ViewProjectionMatrix;
	};
	static std::unique_ptr<SceneData> s_scene_data;
};

}
