#include "dlpch.h"

#include <filesystem>

#include "RenderConstants.h"
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

void Renderer::LoadShaderLibrary(const std::filesystem::path& path, bool recompile)
{
	s_shader_library = RenderCommand::LoadShaderLibrary(path, recompile);
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
	RenderCommand::SetViewport(0, 0, width, height);
}

void Renderer::BeginScene(const OrthographicCamera& camera)
{
	s_scene_data->ProjectionViewMatrix = camera.GetViewProjectionMatrix();
}

void Daedalus::Renderer::BeginScene(const PerspectiveCamera& camera)
{
	s_scene_data->ProjectionViewMatrix = camera.GetProjectionViewMatrix();
}

void Renderer::EndScene()
{
}

void Renderer::Submit(const Shader* shader, const VertexArray* vertex_array, const glm::mat4& transform)
{
	shader->Bind();
	shader->SetMat4(ShaderConstants::SceneProjectionView, s_scene_data->ProjectionViewMatrix);
	shader->SetMat4(ShaderConstants::SceneModel, transform);

	RenderCommand::DrawIndexed(vertex_array);

	shader->Unbind();
}

void Renderer::Submit(const Shader* shader, const Mesh* mesh, const glm::mat4& transform)
{
	shader->Bind();
	shader->SetMat4(ShaderConstants::SceneProjectionView, s_scene_data->ProjectionViewMatrix);
	shader->SetMat4(ShaderConstants::SceneModel, transform);

	if (mesh->GetIndexCount() > 0)
	{
		/* With EBO */
		RenderCommand::DrawIndexed(mesh->GetVertexArray().get());
	}
	else
	{
		/* Without EBO */
		RenderCommand::DrawUnindexed(mesh->GetVertexArray().get(), mesh->GetVertexCount());
	}

	shader->Unbind();
}

void Renderer::Submit(const Shader* shader, const Model* model, const glm::mat4& transform)
{
	shader->Bind();
	shader->SetMat4(ShaderConstants::SceneProjectionView, s_scene_data->ProjectionViewMatrix);
	shader->SetMat4(ShaderConstants::SceneModel, transform);

	const auto& meshes = model->GetMeshes();
	const auto& materials = model->GetMaterials();

	for (const auto& mesh : meshes)
	{
		const auto& material = materials[mesh->GetMaterialIndex()];

		shader->SetFloat3(ShaderConstants::MaterialKAmbient, material.GetAmbientK());
		shader->SetFloat3(ShaderConstants::MaterialKDiffuse, material.GetDiffuseK());
		shader->SetFloat3(ShaderConstants::MaterialKSpecular,material.GetSpecularK());
		shader->SetFloat(ShaderConstants::MaterialKAmbient, material.GetShininess());

		if (const auto& diffuse_map = material.GetDiffuseMap(); diffuse_map)
		{
			diffuse_map->Bind(0);
			shader->SetInt(ShaderConstants::ConfigDiffuseMapUsed, 1);
			shader->SetInt(ShaderConstants::MaterialTexDiffuse, 0);
		}

		else
		{
			RenderCommand::UnbindTextureSlot(0);
			shader->SetInt(ShaderConstants::ConfigDiffuseMapUsed, 0);
		}

		if (const auto& specular_map = material.GetSpecularMap(); specular_map)
		{
			specular_map->Bind(1);
			shader->SetInt(ShaderConstants::ConfigSpecularMapUsed, 1);
			shader->SetInt(ShaderConstants::MaterialTexSpecular, 1);
		}
		else
		{
			RenderCommand::UnbindTextureSlot(1);
			shader->SetInt(ShaderConstants::ConfigSpecularMapUsed, 0);
		}

		if (const auto& normal_map = material.GetDiffuseMap(); normal_map)
		{
			normal_map->Bind(2);
			shader->SetInt(ShaderConstants::ConfigNormalMapUsed, 1);
			shader->SetInt(ShaderConstants::MaterialTexNormal, 2);
		}
		else
		{
			RenderCommand::UnbindTextureSlot(2);
			shader->SetInt(ShaderConstants::ConfigNormalMapUsed, 0);
		}

		const auto vertex_array = mesh->GetVertexArray();
		RenderCommand::DrawIndexed(vertex_array.get());
	}

	shader->Unbind();
}