#include "dlpch.h"
#include "Renderer.h"

using namespace Daedalus;

std::unique_ptr<Renderer::SceneData> Renderer::s_scene_data = std::make_unique<Renderer::SceneData>();

void Renderer::Init()
{
	RenderCommand::Init();
}

void Renderer::Shutdown()
{

}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
	RenderCommand::SetViewport(0, 0, width, height);
}

void Renderer::BeginScene(/*OrthographicCamera& camera*/)
{
	//s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
}

void Renderer::EndScene()
{
}

void Renderer::Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertex_array, const glm::mat4& transform)
{
	shader->Bind();
	//shader->SetMat4("u_ViewProjection", s_scene_data->ViewProjectionMatrix);
	//shader->SetMat4("u_Transform", transform);

	vertex_array->Bind();
	RenderCommand::DrawIndexed(vertex_array);
}
