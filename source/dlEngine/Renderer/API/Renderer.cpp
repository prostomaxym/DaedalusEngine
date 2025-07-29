#include "dlpch.h"

#include "Renderer.h"

#include "Config/GraphicsConfig.h"
#include "Renderer/Resourses/ShaderLibrary.h"
#include "RenderConstants.h"

#include <filesystem>

using namespace Daedalus;

std::unique_ptr<Renderer::Data> Renderer::s_data = nullptr;
std::unique_ptr<Renderer::DebugInfo> Renderer::s_debug = nullptr;

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

	std::shared_ptr<VertexArray> CreateUnitQuad()
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

	constexpr auto DebugEnabled = true;
}

void Renderer::Init()
{
	RenderCommand::Init();

	Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Loading Graphic Settings");
	RenderCommand::SetupGraphicSettings();

	s_data = std::make_unique<Data>();
	s_data->window_width = GraphicsConfig::GetWindowWidth();
	s_data->window_height = GraphicsConfig::GetWindowHeight();

	struct BufferData
	{
		int gamma_enabled = 0;
		float gamma_value = 0.f;
		int pcf_multiplier = 0;
		float csm_exponent = 0.f;
		int enable_ssao = 1;
		int align1 = -1;
		int align2 = -1;
		int align3 = -1;
	};

	BufferData data;
	data.gamma_enabled = GraphicsConfig::IsGammaCorrectionEnabled() ? 1 : 0;
	data.gamma_value = GraphicsConfig::GetGammaCorrectionValue();
	data.pcf_multiplier = GraphicsConfig::GetShadowPCFMultiplier();
	data.csm_exponent = GraphicsConfig::GetShadowCSMExponent();
	data.enable_ssao = GraphicsConfig::IsSSBOEnabled() ? 1 : 0;

	s_data->UBO_graphic_config = UniformBuffer::Create(sizeof(BufferData), 1, UniformBuffer::Type::Static, &data);
	s_data->UBO_scene_data = UniformBuffer::Create(sizeof(float) * 40, 0, UniformBuffer::Type::Dynamic);
	s_data->unit_quad = CreateUnitQuad();

	s_data->geometry_pass.CreateGBuffer(s_data->window_width, s_data->window_height);
	s_data->ssao_pass.CreateSSAOBuffers(s_data->window_width, s_data->window_height);

	s_debug = std::make_unique<DebugInfo>();

	if (DebugEnabled)
	{
		const auto width = GraphicsConfig::GetShadowBufferWidth();
		const auto height = GraphicsConfig::GetShadowBufferHeight();
		s_debug->greyscale_shadow = Texture2D::Create(width, height);
		s_debug->greyscale_depth = Texture2D::Create(s_data->window_width, s_data->window_height);
		s_debug->greyscale_ssao = Texture2D::Create(s_data->window_width, s_data->window_height);
		s_debug->greyscale_shininess = Texture2D::Create(s_data->window_width, s_data->window_height);
	}
}

void Renderer::Shutdown()
{
	s_data.reset();
}

void Renderer::LoadShaderLibrary(const std::filesystem::path& path, bool recompile)
{
	s_data->shader_library = std::make_unique<ShaderLibrary>(path, recompile);
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
	RenderCommand::SetViewport(0, 0, width, height);
	s_data->window_width = width;
	s_data->window_height = height;

	s_data->geometry_pass.CreateGBuffer(width, height);
	s_data->ssao_pass.CreateSSAOBuffers(width, height);
}

void Renderer::BeginFrame(const Camera* camera, std::optional<int> number_of_objects)
{
	const auto PV = camera->GetProjectionViewMatrix();
	const auto V = camera->GetViewMatrix();
	const auto pos = camera->GetPosition();
	const auto znear = camera->GetNearPlane();
	const auto zfar = camera->GetFarPlane();
	s_data->scene_proj = camera->GetProjectionMatrix();
	s_data->scene_view = V;

	s_data->UBO_scene_data->SetData(&PV, sizeof(float) * 16, 0);
	s_data->UBO_scene_data->SetData(&V, sizeof(float) * 16, 64);
	s_data->UBO_scene_data->SetData(&pos, sizeof(float) * 3, 128);
	s_data->UBO_scene_data->SetData(&znear, sizeof(float) * 1, 140);
	s_data->UBO_scene_data->SetData(&zfar, sizeof(float) * 1, 144);
	s_data->view_frustum = camera->GetViewFrustum();
	s_data->zfar = camera->GetFarPlane();
	s_data->znear = camera->GetNearPlane();

	RenderCommand::SetClearColor({ 0.0f, 0.0f, 0.0f, 1.0 });
	s_data->frame_models.clear();

	if (number_of_objects.has_value())
		s_data->frame_models.reserve(number_of_objects.value());
}

void Renderer::FlushPipeline()
{
	UpdateLightShaderData();

	s_debug->shadow = s_data->shadow_pass.Render(ShadowPass::PassIn(s_data->frame_models));
	s_debug->number_of_shadow_maps = s_data->shadow_pass.GetNumberOfShadowMaps();

	s_debug->geom = s_data->geometry_pass.Render(DeferredGeometryPass::PassIn(s_data->frame_models, s_data->view_frustum, s_data->window_width, s_data->window_height));
	DeferredLightPass::PassIn geom_out(s_debug->geom, s_debug->shadow.shadow_map_id, s_data->window_width, s_data->window_height);

	if (GraphicsConfig::IsSSBOEnabled())
	{
		s_debug->ssao = s_data->ssao_pass.Render(SSAOPass::PassIn(s_debug->geom.pos_texture, s_debug->geom.norm_texture, s_data->scene_proj, s_data->scene_view));
		geom_out.ssao_texture = s_debug->ssao.ssao_texture;
	}

	s_data->light_pass.Render(geom_out);

	const auto gbuffer = s_data->geometry_pass.GetBuffer();
	const auto& spec = gbuffer->GetSpecification();
	Framebuffer::CopyDepthFramebuffer(gbuffer->GetID(), 0, spec.width, spec.height);
	s_debug->depth_id = gbuffer->GetDepthAttachmentID();

	ComputeDebugInfo();
}

void Renderer::Draw(const Shader* shader, const VertexArray* vertex_array, const glm::mat4& transform)
{
	shader->SetMat4(ShaderConstants::SceneModel, transform);
	RenderCommand::DrawIndexed(vertex_array);
}

void Renderer::Draw(const Shader* shader, const Mesh* mesh, const glm::mat4& transform)
{
	if (!mesh->IsVisible(s_data->view_frustum, transform))
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

void Renderer::Submit(const Model* model, const glm::mat4& transform)
{
	s_data->frame_models.emplace_back(model, transform );
}

void Renderer::Draw(const Shader* shader, const Cubemap* cubemap, const glm::mat4& transform)
{
	shader->SetMat4(ShaderConstants::CubemapProjectionView, transform);

	const auto& vertex_array = cubemap->GetVertexArray();
	const auto& cubemap_texture = cubemap->GetCubemapTexture();
	cubemap_texture->Bind(0);
	shader->SetInt(ShaderConstants::CubemapTexture, 0);

	RenderCommand::DrawUnindexed(vertex_array.get(), cubemap->GetIndexCount());
}

void Renderer::DrawUnitQuad()
{
	RenderCommand::DrawIndexed(s_data->unit_quad.get());
}

void Renderer::UpdateLightShaderData()
{
	std::vector<LightSSBO> light_SSBOs;
	std::vector<glm::mat4> light_proj_view;

	for (const auto light : s_data->lights)
	{
		if (light->CastShadow())
		{
			light->SetShadowMapIndex(light_proj_view.size());
			const auto cascades = light->CalculateCascadesProjView(s_data->scene_proj, s_data->scene_view);
			light_proj_view.insert(light_proj_view.end(), cascades.begin(), cascades.end());
		}

		light_SSBOs.emplace_back(light->GetShaderSSBO());
	}

	Renderer::UpdateLightSSBO(light_SSBOs);
	Renderer::UpdateLightSpaceMatricesSSBO(light_proj_view);
}

void Renderer::UpdateLightSSBO(const std::vector<LightSSBO>& light_SSBOs)
{
	static std::shared_ptr<ShaderStorageBuffer> SSBO_lighting = nullptr;
	if (light_SSBOs.empty())
	{
		SSBO_lighting.reset();
		return;
	}
		
	const auto SSBO_size_in_bytes = light_SSBOs.size() * sizeof(LightSSBO);
	SSBO_lighting = ShaderStorageBuffer::Create(SSBO_size_in_bytes, 0, ShaderStorageBuffer::Type::Dynamic);
	SSBO_lighting->SetData(light_SSBOs.data(), SSBO_size_in_bytes, 0);
}

void Renderer::UpdateLightSpaceMatricesSSBO(const std::vector<glm::mat4>& light_proj_view)
{
	static std::shared_ptr<ShaderStorageBuffer> SSBO_light_space_matrices = nullptr;

	if (light_proj_view.empty())
	{
		SSBO_light_space_matrices.reset();
		return;
	}
		
	const auto SSBO_size_in_bytes = light_proj_view.size() * sizeof(glm::mat4);
	SSBO_light_space_matrices = ShaderStorageBuffer::Create(SSBO_size_in_bytes, 1, ShaderStorageBuffer::Type::Dynamic);
	SSBO_light_space_matrices->SetData(light_proj_view.data(), SSBO_size_in_bytes, 0);
}

int Renderer::CalculateNumberOfShadowMaps()
{
	int number_of_shadowmaps = 0;
	for (const auto light : s_data->lights)
	{
		if (light->CastShadow())
			number_of_shadowmaps += light->GetShadowNumberOfCascades();
	}

	return number_of_shadowmaps;
}

void Renderer::ComputeDebugInfo()
{
	if (!DebugEnabled)
		return;

	const auto shadow_w = GraphicsConfig::GetShadowBufferWidth();
	const auto shadow_h = GraphicsConfig::GetShadowBufferHeight();
	const auto win_w = s_data->window_width;
	const auto win_h = s_data->window_height;

	ComputeTextureArray(ShaderConstants::ArrayToGrayScaleShader, s_debug->shadow.shadow_map_id, s_debug->greyscale_shadow->GetRendererID(), shadow_w, shadow_h, s_debug->current_shadow_map);
	ComputeDepthTexture(ShaderConstants::DepthToGrayScaleShader, s_debug->depth_id, s_debug->greyscale_depth->GetRendererID(), win_w, s_data->window_height);
	ComputeTexture(ShaderConstants::TextureToGrayScaleShader, s_debug->ssao.ssao_texture, s_debug->greyscale_ssao->GetRendererID(), win_w, win_h);
	ComputeTexture(ShaderConstants::TextureToGrayScaleShader, s_debug->geom.shininess_texture, s_debug->greyscale_shininess->GetRendererID(), win_w, win_h);

	const auto compute_shader = Renderer::GetShaderLibrary()->Get(ShaderConstants::ArrayToGrayScaleShader);
}

void Renderer::ComputeTexture(std::string_view shader, uint32_t id_in, uint32_t id_out, int w, int h)
{
	const auto compute_shader = Renderer::GetShaderLibrary()->Get(shader.data());
	compute_shader->Bind();

	Texture2D::BindTexture(id_in, 0);
	compute_shader->SetInt(ShaderConstants::ComputeInput, 0);

	Texture2D::BindTextureImage(id_out, 1, Texture::ColorFormat::RGBA8, false, true);
	compute_shader->SetInt(ShaderConstants::ComputeOutput, 1);

	compute_shader->DispatchCompute(w, h, 16);
	compute_shader->Unbind();
}

void Renderer::ComputeTextureArray(std::string_view shader, uint32_t id_in, uint32_t id_out, int w, int h, int layer)
{
	const auto compute_shader = Renderer::GetShaderLibrary()->Get(shader.data());
	compute_shader->Bind();

	Texture2D::BindTexture(id_in, 0);
	compute_shader->SetInt(ShaderConstants::ComputeArray, 0);

	Texture2D::BindTextureImage(id_out, 1, Texture::ColorFormat::RGBA8, false, true);
	compute_shader->SetInt(ShaderConstants::ComputeOutput, 1);
	compute_shader->SetInt(ShaderConstants::LayerIndex, layer);

	compute_shader->DispatchCompute(w, h, 16);
	compute_shader->Unbind();
}

void Renderer::ComputeDepthTexture(std::string_view shader, uint32_t id_in, uint32_t id_out, int w, int h)
{
	const auto compute_shader = Renderer::GetShaderLibrary()->Get(shader.data());
	compute_shader->Bind();
	compute_shader->SetFloat(ShaderConstants::Znear, s_data->znear);
	compute_shader->SetFloat(ShaderConstants::Zfar, s_data->zfar);

	Texture2D::BindTexture(id_in, 0);
	compute_shader->SetInt(ShaderConstants::ComputeInput, 0);

	Texture2D::BindTextureImage(id_out, 1, Texture::ColorFormat::RGBA8, false, true);
	compute_shader->SetInt(ShaderConstants::ComputeOutput, 1);

	compute_shader->DispatchCompute(w, h, 16);
	compute_shader->Unbind();
}

void Renderer::SetLights(const std::vector<LightSource*>& lights)
{
	s_data->lights = lights;
	s_data->shadow_pass.SetNumberOfShadowMaps(CalculateNumberOfShadowMaps());
}

void Renderer::AddLight(LightSource* light)
{
	s_data->lights.push_back(light);

	if (light->CastShadow())
		s_data->shadow_pass.SetNumberOfShadowMaps(CalculateNumberOfShadowMaps());
}

void Renderer::RemoveLight(LightSource* light)
{
	auto it = std::find(s_data->lights.begin(), s_data->lights.end(), light);
	if (it != s_data->lights.end())
	{
		s_data->lights.erase(it);

		if (light->CastShadow())
			s_data->shadow_pass.SetNumberOfShadowMaps(CalculateNumberOfShadowMaps());
	}
	else
	{
		DL_ASSERT(false, Log::Categories::Renderer, "Trying to remove not existing light source from renderer");
	}	
}