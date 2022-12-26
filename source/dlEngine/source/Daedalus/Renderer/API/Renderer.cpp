#include "dlpch.h"
#include "Renderer.h"

using namespace Daedalus;

std::unique_ptr<Renderer::SceneData> Renderer::s_scene_data = std::make_unique<Renderer::SceneData>();
std::unique_ptr<ShaderLibrary> Renderer::s_shader_library = std::make_unique<ShaderLibrary>();

void Renderer::Init()
{
	RenderCommand::Init();
}

void Renderer::Shutdown()
{

}

void Renderer::LoadShaderLibrary(const std::string& path, bool recompile)
{
	s_shader_library = RenderCommand::LoadShaderLibrary(path, recompile);
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
	RenderCommand::SetViewport(0, 0, width, height);
}

void Renderer::BeginScene(OrthographicCamera& camera)
{
	s_scene_data->ProjectionViewMatrix = camera.GetViewProjectionMatrix();
}

void Daedalus::Renderer::BeginScene(PerspectiveCamera& camera)
{
	s_scene_data->ProjectionViewMatrix = camera.GetProjectionViewMatrix();
}

void Renderer::EndScene()
{
}

void Renderer::Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertex_array, const glm::mat4& transform)
{
	shader->Bind();
	shader->SetMat4("u_projection_view", s_scene_data->ProjectionViewMatrix);
	shader->SetMat4("u_transform", transform);

	vertex_array->Bind();
	RenderCommand::DrawIndexed(vertex_array);
}

void Renderer::Submit(const std::shared_ptr<Shader>& shader, const Mesh* mesh)
{
	shader->Bind();

	if (mesh->GetIndexCount() > 0)
	{
		/* With EBO */
		RenderCommand::DrawIndexed(mesh->GetVertexArray());
	}
	else
	{
		/* Without EBO */
		RenderCommand::DrawUnindexed(mesh->GetVertexArray(), mesh->GetVertexCount());
	}
}