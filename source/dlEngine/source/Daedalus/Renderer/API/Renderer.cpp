#include "dlpch.h"

#include "Renderer.h"

#include "Daedalus/Config/GraphicsConfig.h"
#include "Daedalus/Renderer/Resourses/ShaderLibrary.h"
#include "RenderConstants.h"

#include <filesystem>

using namespace Daedalus;

std::unique_ptr<ShaderLibrary> Renderer::s_shader_library = std::make_unique<ShaderLibrary>();

std::shared_ptr<UniformBuffer> Renderer::s_UBO_scene_data = nullptr;
std::shared_ptr<UniformBuffer> Renderer::s_UBO_graphic_config = nullptr;
Frustum Renderer::s_view_frustum = Frustum();
glm::mat4 Renderer::s_scene_proj = glm::mat4();
glm::mat4 Renderer::s_scene_view = glm::mat4();

std::map<uint32_t, LightSource*> Renderer::s_lights = std::map<uint32_t, LightSource*>();
std::vector<std::pair<const Model*, glm::mat4>> Renderer::s_frame_models = std::vector<std::pair<const Model*, glm::mat4>>();

std::shared_ptr<VertexArray> Renderer::s_unit_quad = nullptr;

ShadowPass Renderer::s_shadow_pass = ShadowPass();
DeferredGeometryPass Renderer::s_geometry_pass = DeferredGeometryPass();
DeferredLightPass Renderer::s_light_pass = DeferredLightPass();
SSAOPass Renderer::s_ssao_pass = SSAOPass();

int Renderer::s_window_width = 0;
int Renderer::s_window_height = 0;

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
}

void Renderer::Init()
{
	RenderCommand::Init();

	Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Loading Graphic Settings");
	RenderCommand::SetupGraphicSettings();

	s_window_width = GraphicsConfig::GetWindowWidth();
	s_window_height = GraphicsConfig::GetWindowHeight();

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

	s_UBO_graphic_config = UniformBuffer::Create(sizeof(BufferData), 1, UniformBuffer::Type::Static, &data);
	s_UBO_scene_data = UniformBuffer::Create(sizeof(float) * 40, 0, UniformBuffer::Type::Dynamic);
	s_unit_quad = CreateUnitQuad();

	s_geometry_pass.CreateGBuffer(s_window_width, s_window_height);
	s_ssao_pass.CreateSSAOBuffers(s_window_width, s_window_height);
}

void Renderer::Shutdown()
{

}

void Renderer::LoadShaderLibrary(const std::filesystem::path& path, bool recompile)
{
	s_shader_library = std::make_unique<ShaderLibrary>(path, recompile);
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
	RenderCommand::SetViewport(0, 0, width, height);
	s_window_width = width;
	s_window_height = height;

	s_geometry_pass.CreateGBuffer(width, height);
	s_ssao_pass.CreateSSAOBuffers(width, height);
}

void Renderer::BeginFrame(const Camera* camera, std::optional<int> number_of_objects)
{
	const auto PV = camera->GetProjectionViewMatrix();
	const auto V = camera->GetViewMatrix();
	const auto pos = camera->GetPosition();
	const auto znear = camera->GetNearPlane();
	const auto zfar = camera->GetFarPlane();
	s_scene_proj = camera->GetProjectionMatrix();
	s_scene_view = V;

	s_UBO_scene_data->SetData(&PV, sizeof(float) * 16, 0);
	s_UBO_scene_data->SetData(&V, sizeof(float) * 16, 64);
	s_UBO_scene_data->SetData(&pos, sizeof(float) * 3, 128);
	s_UBO_scene_data->SetData(&znear, sizeof(float) * 1, 140);
	s_UBO_scene_data->SetData(&zfar, sizeof(float) * 1, 144);
	s_view_frustum = camera->GetViewFrustum();

	RenderCommand::SetClearColor({ 0.0f, 0.0f, 0.0f, 1.0 });
	s_frame_models.clear();

	if (number_of_objects.has_value())
		s_frame_models.reserve(number_of_objects.value());
}

void Renderer::FlushPipeline()
{
	UpdateLightShaderData();

	const auto shadow_output = s_shadow_pass.Render(ShadowPass::PassIn(s_frame_models));

	const auto geometry_output = s_geometry_pass.Render(DeferredGeometryPass::PassIn(s_frame_models, s_view_frustum, s_window_width, s_window_height));
	DeferredLightPass::PassIn geom_out(geometry_output, shadow_output.shadow_map_id, s_window_width, s_window_height);

	if (GraphicsConfig::IsSSBOEnabled())
	{
		const auto ssao_output = s_ssao_pass.Render(SSAOPass::PassIn(geometry_output.pos_texture, geometry_output.norm_texture, s_scene_proj, s_scene_view));
		geom_out.ssao_texture = ssao_output.ssao_texture;
	}

	s_light_pass.Render(geom_out);

	//const auto gbuffer = s_geometry_pass.GetBuffer();
	//const auto& spec = gbuffer->GetSpecification();
	//Framebuffer::CopyDepthFramebuffer(gbuffer->GetID(), 0, spec.width, spec.height);
}

void Renderer::Draw(const Shader* shader, const VertexArray* vertex_array, const glm::mat4& transform)
{
	shader->SetMat4(ShaderConstants::SceneModel, transform);
	RenderCommand::DrawIndexed(vertex_array);
}

void Renderer::Draw(const Shader* shader, const Mesh* mesh, const glm::mat4& transform)
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

void Renderer::Submit(const Model* model, const glm::mat4& transform)
{
	s_frame_models.emplace_back(std::make_pair(model, transform ));
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
	RenderCommand::DrawIndexed(s_unit_quad.get());
}

void Renderer::UpdateLightShaderData()
{
	std::vector<LightSSBO> light_SSBOs;
	std::vector<glm::mat4> light_proj_view;

	for (const auto light_pair : s_lights)
	{
		const auto light = light_pair.second;
		if (light->CastShadow())
		{
			light->SetShadowMapIndex(light_proj_view.size());
			const auto cascades = light->CalculateCascadesProjView(s_scene_proj, s_scene_view);
			light_proj_view.insert(light_proj_view.end(), cascades.begin(), cascades.end());
		}

		light_SSBOs.emplace_back(light->GetShaderSSBO());
	}

	Renderer::UpdateLightSpaceMatricesSSBO(light_proj_view);
	Renderer::UpdateLightSSBO(light_SSBOs);
}

void Renderer::UpdateLightSSBO(const std::vector<LightSSBO>& light_SSBOs)
{
	if (light_SSBOs.empty())
		return;

	const auto SSBO_size_in_bytes = light_SSBOs.size() * sizeof(LightSSBO);
	auto SSBO_lighting = ShaderStorageBuffer::Create(SSBO_size_in_bytes, 0, ShaderStorageBuffer::Type::Dynamic);
	SSBO_lighting->SetData(light_SSBOs.data(), SSBO_size_in_bytes, 0);
}

void Renderer::UpdateLightSpaceMatricesSSBO(const std::vector<glm::mat4>& light_proj_view)
{
	if (light_proj_view.empty())
		return;

	const auto SSBO_size_in_bytes = light_proj_view.size() * sizeof(glm::mat4);
	auto SSBO_light_space_matrices = ShaderStorageBuffer::Create(SSBO_size_in_bytes, 1, ShaderStorageBuffer::Type::Dynamic);
	SSBO_light_space_matrices->SetData(light_proj_view.data(), SSBO_size_in_bytes, 0);
}

void Renderer::SetLights(std::map<uint32_t, LightSource*> lights)
{
	s_lights = lights;

	int number_of_shadowmaps = 0;
	for (const auto light_pair : s_lights)
	{
		const auto light = light_pair.second;
		if (light->CastShadow())
			number_of_shadowmaps += light->GetShadowNumberOfCascades();
	}

	s_shadow_pass.SetNumberOfShadowMaps(number_of_shadowmaps);
}

void Renderer::AddLight(uint32_t id, LightSource* light)
{
	s_lights[id] = light;
}

void Renderer::RemoveLight(uint32_t id)
{
	s_lights.erase(id);
}