#include "dlpch.h"

#include "Renderer.h"

#include "Daedalus/Config/GraphicsConfig.h"
#include "Daedalus/Renderer/Resourses/ShaderLibrary.h"
#include "RenderConstants.h"

#include <filesystem>

using namespace Daedalus;

std::unique_ptr<ShaderLibrary> Renderer::s_shader_library = std::make_unique<ShaderLibrary>();

std::unique_ptr<SSAO> Renderer::s_ssao = nullptr;

std::shared_ptr<UniformBuffer> Renderer::s_UBO_scene_data = nullptr;
std::shared_ptr<UniformBuffer> Renderer::s_UBO_graphic_config = nullptr;

std::shared_ptr<ShaderStorageBuffer> Renderer::s_SSBO_light_space_matrices = nullptr;
std::shared_ptr<ShaderStorageBuffer> Renderer::s_SSBO_lighting = nullptr;

std::shared_ptr<Framebuffer> Renderer::s_framebuffer_shadows = nullptr;
std::shared_ptr<Framebuffer> Renderer::s_g_framebuffer = nullptr;

std::shared_ptr<VertexArray> Renderer::s_unit_quad = nullptr;

Frustum Renderer::s_view_frustum = Frustum();
glm::mat4 Renderer::s_light_projection_view = glm::mat4();

namespace
{
	std::array<float, 20> quad_vertices =
	{
		// positions        // texture coords
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};

	std::array<uint32_t, 6> quad_indices =
	{
		0, 1, 2, // first triangle
		1, 3, 2  // second triangle
	};
}

void Renderer::Init()
{
	RenderCommand::Init();

	s_UBO_scene_data = UniformBuffer::Create(sizeof(float) * 40, 0, UniformBuffer::Type::Dynamic);

	FramebufferSpecification shadow_specs;
	shadow_specs.width = GraphicsConfig::GetShadowBufferWidth();
	shadow_specs.height = GraphicsConfig::GetShadowBufferHeight();
	shadow_specs.samples = GraphicsConfig::GetShadowBufferSamples();
	shadow_specs.attachments = FramebufferAttachmentSpecification({ FramebufferTextureSpecification(FramebufferTextureFormat::Depth) });
	shadow_specs.layers = -1;
	s_framebuffer_shadows = Framebuffer::Create(shadow_specs);

	FramebufferSpecification gbuffer_specs;
	gbuffer_specs.width = GraphicsConfig::GetWindowWidth();
	gbuffer_specs.height = GraphicsConfig::GetWindowHeight();
	gbuffer_specs.samples = 1;
	gbuffer_specs.attachments = FramebufferAttachmentSpecification({
		FramebufferTextureSpecification(FramebufferTextureFormat::RGBA16F),
		FramebufferTextureSpecification(FramebufferTextureFormat::RGBA16F),
		FramebufferTextureSpecification(FramebufferTextureFormat::RGBA32U),
		FramebufferTextureSpecification(FramebufferTextureFormat::RGBA32U),
		FramebufferTextureSpecification(FramebufferTextureFormat::RGBA32U),
		FramebufferTextureSpecification(FramebufferTextureFormat::RED16F),
		FramebufferTextureSpecification(FramebufferTextureFormat::Depth) });
	gbuffer_specs.layers = -1;
	s_g_framebuffer = Framebuffer::Create(gbuffer_specs);

	s_unit_quad = CreateUnitQuad();
	s_ssao = std::make_unique<SSAO>(GraphicsConfig::GetWindowWidth(), GraphicsConfig::GetWindowHeight(), GraphicsConfig::GetSSAOKernelSize(), 4, 4);
}

void Renderer::Shutdown()
{

}

void Renderer::SetupGraphicSettings()
{
	Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Loading Graphic Settings");
	RenderCommand::SetupGraphicSettings();

	struct BufferData
	{
		int gamma_enabled = 0;
		float gamma_value = 0.f;
		int pcf_multiplier = 0;
		float csm_exponent = 0.f;
		bool enable_ssao = true;
		int align1 = -1;
		int align2= -1;
		int align3= -1;
	};

	BufferData data;
	data.gamma_enabled = GraphicsConfig::IsGammaCorrectionEnabled() ? 1 : 0;
	data.gamma_value = GraphicsConfig::GetGammaCorrectionValue();
	data.pcf_multiplier = GraphicsConfig::GetShadowPCFMultiplier();
	data.csm_exponent = GraphicsConfig::GetShadowCSMExponent();

	s_UBO_graphic_config = UniformBuffer::Create(sizeof(BufferData), 1, UniformBuffer::Type::Static, &data);
	s_ssao->CreateUBO();
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
	const auto znear = camera->GetNearPlane();
	const auto zfar = camera->GetFarPlane();

	s_UBO_scene_data->SetData(&PV, sizeof(float) * 16, 0);
	s_UBO_scene_data->SetData(&V, sizeof(float) * 16, 64);
	s_UBO_scene_data->SetData(&pos, sizeof(float) * 3, 128);
	s_UBO_scene_data->SetData(&znear, sizeof(float) * 1, 140);
	s_UBO_scene_data->SetData(&zfar, sizeof(float) * 1, 144);
	s_view_frustum = camera->GetViewFrustum();
}

void Renderer::EndScene()
{
}

void Renderer::Submit(const Shader* shader, const VertexArray* vertex_array, const glm::mat4& transform)
{
	shader->SetMat4(ShaderConstants::SceneModel, transform);
	RenderCommand::DrawIndexed(vertex_array);
}

void Renderer::Submit(const Shader* shader, const Mesh* mesh, const glm::mat4& transform)
{
	if (!mesh->IsVisible(s_view_frustum, transform))
		return;

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
}

void Renderer::Submit(const Shader* shader, const Model* model, const glm::mat4& transform)
{
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

		if (const auto& height_map = material.GetHeightMap(); height_map)
		{
			height_map->Bind(3);
			shader->SetInt(ShaderConstants::ConfigHeightMapUsed, 1);
			shader->SetInt(ShaderConstants::MaterialTexHeight, 3);
		}
		else
		{
			RenderCommand::UnbindTextureSlot(3);
			shader->SetInt(ShaderConstants::ConfigHeightMapUsed, 0);
		}

		const auto vertex_array = mesh->GetVertexArray();
		RenderCommand::DrawIndexed(vertex_array.get());
	}
}

void Renderer::Submit(const Shader* shader, const Cubemap* cubemap, const glm::mat4& transform)
{
	shader->SetMat4(ShaderConstants::CubemapProjectionView, transform);

	const auto& vertex_array = cubemap->GetVertexArray();
	const auto& cubemap_texture = cubemap->GetCubemapTexture();
	cubemap_texture->Bind(0);
	shader->SetInt(ShaderConstants::CubemapTexture, 0);

	RenderCommand::DrawUnindexed(vertex_array.get(), cubemap->GetIndexCount());
}

void Renderer::SubmitForShadowBuffer(const Shader* shader, const Model* model, const glm::mat4& transform)
{
	shader->SetMat4(ShaderConstants::ShadowModel, transform);

	const auto& meshes = model->GetMeshes();

	for (const auto& mesh : meshes)
	{
		const auto vertex_array = mesh->GetVertexArray();
		RenderCommand::DrawIndexed(vertex_array.get());
	}
}

void Renderer::DrawUnitQuad()
{
	RenderCommand::DrawIndexed(s_unit_quad.get());
}

void Renderer::UpdateLightSSBO(const std::vector<LightSSBO>& light_SSBOs)
{
	if (light_SSBOs.empty())
	{
		s_SSBO_lighting.reset();
		return;
	}

	const auto SSBO_size_in_bytes = light_SSBOs.size() * sizeof(LightSSBO);
	s_SSBO_lighting = ShaderStorageBuffer::Create(SSBO_size_in_bytes, 0, ShaderStorageBuffer::Type::Dynamic);
	s_SSBO_lighting->SetData(light_SSBOs.data(), SSBO_size_in_bytes, 0);
}

void Renderer::UpdateLightSpaceMatricesSSBO(const std::vector<glm::mat4>& light_proj_view)
{
	if (light_proj_view.empty())
	{
		s_SSBO_light_space_matrices.reset();
		return;
	}

	const auto SSBO_size_in_bytes = light_proj_view.size() * sizeof(glm::mat4);
	s_SSBO_light_space_matrices = ShaderStorageBuffer::Create(SSBO_size_in_bytes, 1, ShaderStorageBuffer::Type::Dynamic);
	s_SSBO_light_space_matrices->SetData(light_proj_view.data(), SSBO_size_in_bytes, 0);
}

void Renderer::UpdateNumberOfShadowMap(int number_of_shadow_map)
{
	FramebufferSpecification specs;
	specs.width = GraphicsConfig::GetShadowBufferWidth();
	specs.height = GraphicsConfig::GetShadowBufferHeight();
	specs.samples = GraphicsConfig::GetShadowBufferSamples();
	specs.attachments = FramebufferAttachmentSpecification({ FramebufferTextureSpecification(FramebufferTextureFormat::Depth) });
	specs.layers = number_of_shadow_map;

	s_framebuffer_shadows = Framebuffer::Create(specs);
}

void Renderer::BindShadowMap(const Shader* color_pass_shader, int slot)
{
	Texture2D::BindTexture(s_framebuffer_shadows->GetDepthAttachmentID(), slot);
	color_pass_shader->SetInt(ShaderConstants::ShadowMaps, slot);
}

void Renderer::BindSSAOTextures(const Shader* ssao_pass_shader, const glm::mat4& proj, const glm::mat4& view)
{
	Texture2D::BindTexture(s_g_framebuffer->GetColorAttachmentRendererID(0), 0);
	ssao_pass_shader->SetInt(ShaderConstants::GBufferPos, 0);

	Texture2D::BindTexture(s_g_framebuffer->GetColorAttachmentRendererID(1), 1);
	ssao_pass_shader->SetInt(ShaderConstants::GBufferNorm, 1);

	Texture2D::BindTexture(s_ssao->GetNoiseTexture()->GetRendererID(), 2);
	ssao_pass_shader->SetInt(ShaderConstants::SSBOBufferNoise, 2);

	ssao_pass_shader->SetMat4(ShaderConstants::SSBOProjection, proj);
	ssao_pass_shader->SetMat4("u_view", view);
}

void Renderer::BindSSAOBlurTextures(const Shader* blur_pass_shader)
{
	Texture2D::BindTexture(s_ssao->GetSSAOFramebuffer()->GetColorAttachmentRendererID(0), 0);
	blur_pass_shader->SetInt(ShaderConstants::SSBOBlurBufferNoise, 0);
}

void Renderer::BindGBufferTextures(const Shader* light_pass_shader, int first_slot)
{
	Texture2D::BindTexture(s_g_framebuffer->GetColorAttachmentRendererID(0), first_slot);
	light_pass_shader->SetInt(ShaderConstants::GBufferPos, first_slot);

	Texture2D::BindTexture(s_g_framebuffer->GetColorAttachmentRendererID(1), first_slot + 1);
	light_pass_shader->SetInt(ShaderConstants::GBufferNorm, first_slot + 1);

	Texture2D::BindTexture(s_g_framebuffer->GetColorAttachmentRendererID(2), first_slot + 2);
	light_pass_shader->SetInt(ShaderConstants::GBufferAmbient, first_slot + 2);

	Texture2D::BindTexture(s_g_framebuffer->GetColorAttachmentRendererID(3), first_slot + 3);
	light_pass_shader->SetInt(ShaderConstants::GBufferSpec, first_slot + 3);

	Texture2D::BindTexture(s_g_framebuffer->GetColorAttachmentRendererID(4), first_slot + 4);
	light_pass_shader->SetInt(ShaderConstants::GBufferAlbedo, first_slot + 4);

	Texture2D::BindTexture(s_g_framebuffer->GetColorAttachmentRendererID(5), first_slot + 5);
	light_pass_shader->SetInt(ShaderConstants::GBufferShininess, first_slot + 5);

	if (GraphicsConfig::IsSSBOEnabled())
	{
		Texture2D::BindTexture(s_ssao->GetBlurFramebuffer()->GetColorAttachmentRendererID(0), first_slot + 6);
		light_pass_shader->SetInt(ShaderConstants::SSBOFinalBuffer, first_slot + 6);
	}
}

std::shared_ptr<VertexArray> Renderer::CreateUnitQuad()
{
	auto vertex_array = VertexArray::Create();
	vertex_array->Bind();

	auto vertex_buffer = VertexBuffer::Create(quad_vertices.data(), quad_vertices.size() * sizeof(float));
	vertex_buffer->SetLayout(BufferLayout
		{
			BufferElement{ ShaderDataType::Float3, std::string(ShaderConstants::VerticesVar), false },
			BufferElement{ ShaderDataType::Float2, std::string(ShaderConstants::TexCoordVar), false },
		});

	auto indexes_buffer = IndexBuffer::Create(quad_indices.data(), quad_indices.size());

	vertex_array->AddVertexBuffer(vertex_buffer);
	vertex_array->SetIndexBuffer(indexes_buffer);
	vertex_array->Unbind();

	return vertex_array;
}