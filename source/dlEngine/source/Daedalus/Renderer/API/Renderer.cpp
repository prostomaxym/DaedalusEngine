#include "dlpch.h"

#include <filesystem>

#include "RenderConstants.h"
#include "Renderer.h"

using namespace Daedalus;

std::unique_ptr<ShaderLibrary> Renderer::s_shader_library = std::make_unique<ShaderLibrary>();
std::shared_ptr<UniformBuffer> Renderer::s_UBO_scene_data;
std::shared_ptr<UniformBuffer> Renderer::s_UBO_graphic_config;
std::shared_ptr<ShaderStorageBuffer> Renderer::s_SSBO_static_lighting = nullptr;
std::shared_ptr<ShaderStorageBuffer> Renderer::s_SSBO_dynamic_lighting = nullptr;
std::shared_ptr<Framebuffer> Renderer::s_framebuffer_shadows = nullptr;


namespace
{
	glm::mat4 GetLightMatrix()
	{
		glm::mat4 lightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, -1000.f, 1000.f);

		glm::mat4 lightView = glm::lookAt(glm::vec3(0.78f, 1.0f, 0.6f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f));

		return lightProjection * lightView;
	}
}
void Renderer::Init()
{
	RenderCommand::Init();

	s_UBO_scene_data = UniformBuffer::Create(sizeof(float) * 20, 0, UniformBuffer::Type::Dynamic);

	FramebufferSpecification specs;
	specs.width = 2048;
	specs.height = 2048;
	specs.samples = 1;
	specs.attachments = FramebufferAttachmentSpecification({ FramebufferTextureSpecification(FramebufferTextureFormat::Depth) });

	s_framebuffer_shadows = Framebuffer::Create(specs);
}

void Renderer::Shutdown()
{

}

void Renderer::SetupGraphicSettings()
{
	RenderCommand::SetupGraphicSettings();

	//TODO: create config class
	s_UBO_graphic_config = UniformBuffer::Create(sizeof(float) * 4, 1, UniformBuffer::Type::Static);

	int gamma_enabled = 0;
	float gamma_value = 2.2f;

	s_UBO_graphic_config->SetData(&gamma_enabled, sizeof(int), 0);
	s_UBO_graphic_config->SetData(&gamma_value, sizeof(float), 4);
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
	s_UBO_scene_data->SetData(&camera.GetViewProjectionMatrix(), sizeof(float) * 16, 0);
	s_UBO_scene_data->SetData(&camera.GetPosition(), sizeof(float) * 4, 64);
}

void Renderer::BeginScene(const PerspectiveCamera& camera)
{
	s_UBO_scene_data->SetData(&camera.GetProjectionViewMatrix(), sizeof(float) * 16, 0);
	s_UBO_scene_data->SetData(&camera.GetPosition(), sizeof(float) * 4, 64);
}

void Renderer::EndScene()
{
}

void Renderer::Submit(const Shader* shader, const VertexArray* vertex_array, const glm::mat4& transform)
{
	shader->Bind();
	shader->SetMat4(ShaderConstants::SceneModel, transform);

	RenderCommand::DrawIndexed(vertex_array);

	shader->Unbind();
}

void Renderer::Submit(const Shader* shader, const Mesh* mesh, const glm::mat4& transform)
{
	shader->Bind();
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
	glm::mat4 lightSpaceMatrix = GetLightMatrix();

	shader->Bind();
	shader->SetMat4(ShaderConstants::SceneModel, transform);
	shader->SetMat4(ShaderConstants::ShadowLightSpace, lightSpaceMatrix);

	const auto& meshes = model->GetMeshes();
	const auto& materials = model->GetMaterials();

	for (const auto& mesh : meshes)
	{
		const auto& material = materials[mesh->GetMaterialIndex()];

		shader->SetFloat3(ShaderConstants::MaterialKAmbient, material.GetAmbientK());
		shader->SetFloat3(ShaderConstants::MaterialKDiffuse, material.GetDiffuseK());
		shader->SetFloat3(ShaderConstants::MaterialKSpecular,material.GetSpecularK());
		shader->SetFloat(ShaderConstants::MaterialShininess, material.GetShininess());

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

		if (const auto& normal_map = material.GetNormalMap(); normal_map)
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

void Renderer::Submit(const Shader* shader, const Cubemap* cubemap, const glm::mat4& transform)
{
	shader->Bind();
	shader->SetMat4(ShaderConstants::CubemapProjectionView, transform);

	const auto& vertex_array = cubemap->GetVertexArray();
	const auto& cubemap_texture = cubemap->GetCubemapTexture();
	cubemap_texture->Bind(0);
	shader->SetInt(ShaderConstants::CubemapTexture, 0);

	RenderCommand::DrawUnindexed(vertex_array.get(), cubemap->GetIndexCount());

	shader->Unbind();
}

void Renderer::SubmitForShadowBuffer(const Shader* shader, const Model* model, const glm::mat4& transform)
{
	glm::mat4 lightSpaceMatrix = GetLightMatrix();

	shader->Bind();
	shader->SetMat4(ShaderConstants::ShadowModel, transform);
	shader->SetMat4(ShaderConstants::ShadowLightSpace, lightSpaceMatrix);

	const auto& meshes = model->GetMeshes();

	for (const auto& mesh : meshes)
	{
		const auto vertex_array = mesh->GetVertexArray();
		RenderCommand::DrawIndexed(vertex_array.get());
	}

	shader->Unbind();
}

void Renderer::UpdateStaticLightSSBO(const std::vector<LightSSBO>& light_SSBOs)
{
	if (light_SSBOs.empty())
		return;

	const auto SSBO_size_in_bytes = light_SSBOs.size() * sizeof(LightSSBO);
	s_SSBO_static_lighting = ShaderStorageBuffer::Create(SSBO_size_in_bytes, 0, ShaderStorageBuffer::Type::Static);
	s_SSBO_static_lighting->SetData(light_SSBOs.data(), SSBO_size_in_bytes, 0);
}

void Renderer::UpdateDynamicLightSSBO(const std::vector<LightSSBO>& light_SSBOs)
{
	if (light_SSBOs.empty())
		return;

	const auto SSBO_size_in_bytes = light_SSBOs.size() * sizeof(LightSSBO);
	s_SSBO_dynamic_lighting = ShaderStorageBuffer::Create(SSBO_size_in_bytes, 1, ShaderStorageBuffer::Type::Dynamic);
	s_SSBO_dynamic_lighting->SetData(light_SSBOs.data(), SSBO_size_in_bytes, 0);
}

void Renderer::UpdateShadowMap()
{
	auto standard_shader = Renderer::s_shader_library->Get("Standard");
	standard_shader->Bind();
	Texture2D::BindTexture(s_framebuffer_shadows->GetDepthAttachmentID(), 3);
	standard_shader->SetInt(ShaderConstants::ShadowMap, 3);
}
