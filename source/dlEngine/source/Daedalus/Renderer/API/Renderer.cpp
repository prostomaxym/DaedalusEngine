#include "dlpch.h"

#include "Renderer.h"

#include "Daedalus/Config/GraphicsConfig.h"
#include "Daedalus/Renderer/Resourses/ShaderLibrary.h"
#include "RenderConstants.h"

#include <filesystem>

using namespace Daedalus;

std::unique_ptr<ShaderLibrary> Renderer::s_shader_library = std::make_unique<ShaderLibrary>();
std::shared_ptr<UniformBuffer> Renderer::s_UBO_scene_data;
std::shared_ptr<UniformBuffer> Renderer::s_UBO_graphic_config;
std::shared_ptr<ShaderStorageBuffer> Renderer::s_SSBO_light_space_matrices;
std::shared_ptr<ShaderStorageBuffer> Renderer::s_SSBO_static_lighting = nullptr;
std::shared_ptr<ShaderStorageBuffer> Renderer::s_SSBO_dynamic_lighting = nullptr;
std::shared_ptr<Framebuffer> Renderer::s_framebuffer_shadows = nullptr;
Frustum Renderer::s_view_frustum = Frustum();
glm::mat4 Renderer::s_light_projection_view = glm::mat4();

void Renderer::Init()
{
	RenderCommand::Init();

	s_UBO_scene_data = UniformBuffer::Create(sizeof(float) * 40, 0, UniformBuffer::Type::Dynamic);

	FramebufferSpecification specs;
	specs.width = GraphicsConfig::GetShadowBufferWidth();
	specs.height = GraphicsConfig::GetShadowBufferHeight();
	specs.samples = GraphicsConfig::GetShadowBufferSamples();
	specs.attachments = FramebufferAttachmentSpecification({ FramebufferTextureSpecification(FramebufferTextureFormat::Depth) });
	specs.layers = -1;

	s_framebuffer_shadows = Framebuffer::Create(specs);
}

void Renderer::Shutdown()
{

}

void Renderer::SetupGraphicSettings()
{
	RenderCommand::SetupGraphicSettings();

	s_UBO_graphic_config = UniformBuffer::Create(sizeof(float) * 4, 1, UniformBuffer::Type::Static);

	int gamma_enabled = GraphicsConfig::IsGammaCorrectionEnabled() ? 1 : 0;
	float gamma_value = GraphicsConfig::GetGammaCorrectionValue();
	int pcf_multiplier = GraphicsConfig::GetShadowPCFMultiplier();

	s_UBO_graphic_config->SetData(&gamma_enabled, sizeof(int), 0);
	s_UBO_graphic_config->SetData(&gamma_value, sizeof(float), 4);
	s_UBO_graphic_config->SetData(&pcf_multiplier, sizeof(int), 8);
}

void Renderer::LoadShaderLibrary(const std::filesystem::path& path, bool recompile)
{
	s_shader_library = std::make_unique<ShaderLibrary>(path, recompile);
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
	RenderCommand::SetViewport(0, 0, width, height);
}

void Renderer::BeginScene(const Camera* camera)
{
	const auto PV = camera->GetProjectionViewMatrix();
	const auto V = camera->GetViewMatrix();
	const auto pos = camera->GetPosition();
	const auto perc = RendererConstants::CascadePercents;
	const auto cascades = camera->GetCascadeDistances(perc.x, perc.y, perc.z);

	s_UBO_scene_data->SetData(&PV, sizeof(float) * 16, 0);
	s_UBO_scene_data->SetData(&V, sizeof(float) * 16, 64);
	s_UBO_scene_data->SetData(&pos, sizeof(float) * 3, 128);
	s_UBO_scene_data->SetData(&cascades, sizeof(float) * 3, 144);
	s_view_frustum = camera->GetViewFrustum();
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
	if (!mesh->IsVisible(s_view_frustum, transform))
		return;

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
	shader->Bind();
	shader->SetMat4(ShaderConstants::SceneModel, transform);

	const auto& meshes = model->GetMeshes();
	const auto& materials = model->GetMaterials();

	for (const auto& mesh : meshes)
	{
		if (!mesh->IsVisible(s_view_frustum, transform))
			continue;

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
	shader->Bind();
	shader->SetMat4(ShaderConstants::ShadowModel, transform);

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
	{
		s_SSBO_static_lighting.reset();
		return;
	}

	const auto SSBO_size_in_bytes = light_SSBOs.size() * sizeof(LightSSBO);
	s_SSBO_static_lighting = ShaderStorageBuffer::Create(SSBO_size_in_bytes, 0, ShaderStorageBuffer::Type::Static);
	s_SSBO_static_lighting->SetData(light_SSBOs.data(), SSBO_size_in_bytes, 0);
}

void Renderer::UpdateDynamicLightSSBO(const std::vector<LightSSBO>& light_SSBOs)
{
	if (light_SSBOs.empty())
	{
		s_SSBO_dynamic_lighting.reset();
		return;
	}

	const auto SSBO_size_in_bytes = light_SSBOs.size() * sizeof(LightSSBO);
	s_SSBO_dynamic_lighting = ShaderStorageBuffer::Create(SSBO_size_in_bytes, 1, ShaderStorageBuffer::Type::Dynamic);
	s_SSBO_dynamic_lighting->SetData(light_SSBOs.data(), SSBO_size_in_bytes, 0);
}

void Renderer::UpdateLightSpaceMatricesSSBO(const std::vector<glm::mat4>& light_proj_view, const std::vector<glm::mat4>& light_view)
{
	if (light_proj_view.empty() || light_view.empty())
	{
		s_SSBO_light_space_matrices.reset();
		return;
	}

	auto SSBO_size_in_bytes = light_proj_view.size() * sizeof(glm::mat4);
	s_SSBO_light_space_matrices = ShaderStorageBuffer::Create(SSBO_size_in_bytes, 2, ShaderStorageBuffer::Type::Dynamic);
	s_SSBO_light_space_matrices->SetData(light_proj_view.data(), SSBO_size_in_bytes, 0);

	SSBO_size_in_bytes = light_view.size() * sizeof(glm::mat4);
	s_SSBO_light_space_matrices = ShaderStorageBuffer::Create(SSBO_size_in_bytes, 3, ShaderStorageBuffer::Type::Dynamic);
	s_SSBO_light_space_matrices->SetData(light_view.data(), SSBO_size_in_bytes, 0);
}

void Renderer::UpdateNumberOfShadowCasters(int number_of_shadow_casters)
{
	FramebufferSpecification specs;
	specs.width = GraphicsConfig::GetShadowBufferWidth();
	specs.height = GraphicsConfig::GetShadowBufferHeight();
	specs.samples = GraphicsConfig::GetShadowBufferSamples();
	specs.attachments = FramebufferAttachmentSpecification({ FramebufferTextureSpecification(FramebufferTextureFormat::Depth) });
	specs.layers = number_of_shadow_casters * RendererConstants::NumberOfShadowCascades;

	s_framebuffer_shadows = Framebuffer::Create(specs);
}

void Renderer::BindShadowMap(const Shader* color_pass_shader)
{
	color_pass_shader->Bind();

	Texture2D::BindTexture(s_framebuffer_shadows->GetDepthAttachmentID(), 3);
	color_pass_shader->SetInt(ShaderConstants::ShadowMaps, 3);

	color_pass_shader->Unbind();
}