#include "dlpch.h"

#include <filesystem>

#include "Daedalus/Config/GraphicsConfig.h"
#include "RenderConstants.h"
#include "Renderer.h"

using namespace Daedalus;

std::unique_ptr<ShaderLibrary> Renderer::s_shader_library = std::make_unique<ShaderLibrary>();
std::shared_ptr<UniformBuffer> Renderer::s_UBO_scene_data;
std::shared_ptr<UniformBuffer> Renderer::s_UBO_graphic_config;
std::shared_ptr<ShaderStorageBuffer> Renderer::s_SSBO_static_lighting = nullptr;
std::shared_ptr<ShaderStorageBuffer> Renderer::s_SSBO_dynamic_lighting = nullptr;
std::shared_ptr<Framebuffer> Renderer::s_framebuffer_shadows = nullptr;
Frustum Renderer::s_view_frustum = Frustum();
glm::mat4 Renderer::s_light_projection_view = glm::mat4();

namespace
{
	glm::mat4 CalculateLightMatrix(const glm::mat4& proj, const glm::mat4& view)
	{
		const auto inv = glm::inverse(proj * view);

		std::array<glm::vec4, 8> frustumCorners;
		for (unsigned int x = 0, i = 0; x < 2; ++x)
		{
			for (unsigned int y = 0; y < 2; ++y)
			{
				for (unsigned int z = 0; z < 2; ++z)
				{
					const glm::vec4 pt =
						inv * glm::vec4(
							2.0f * x - 1.0f,
							2.0f * y - 1.0f,
							2.0f * z - 1.0f,
							1.0f);
					frustumCorners[i] = (pt / pt.w);
					i++;
				}
			}
		}

		glm::vec3 center = glm::vec3(0, 0, 0);
		for (const auto& v : frustumCorners)
		{
			center += glm::vec3(v);
		}
		center /= frustumCorners.size();

		const auto lightView = glm::lookAt(
			center + glm::vec3(0.78f, 1.0f, 0.6f),
			center,
			glm::vec3(0.0f, 1.0f, 0.0f)
		);

		float minX = std::numeric_limits<float>::max();
		float maxX = std::numeric_limits<float>::lowest();
		float minY = std::numeric_limits<float>::max();
		float maxY = std::numeric_limits<float>::lowest();
		float minZ = std::numeric_limits<float>::max();
		float maxZ = std::numeric_limits<float>::lowest();

		for (const auto& v : frustumCorners)
		{
			const auto trf = lightView * v;
			minX = std::min(minX, trf.x);
			maxX = std::max(maxX, trf.x);
			minY = std::min(minY, trf.y);
			maxY = std::max(maxY, trf.y);
			minZ = std::min(minZ, trf.z);
			maxZ = std::max(maxZ, trf.z);
		}

		// Tune this parameter according to the scene
		constexpr float xMult = 1.2f;
		constexpr float yMult = 1.2f;
		constexpr float zMult = 1.2f;

		if (minX < 0)
			minX *= xMult;
		else
			minX /= xMult;

		if (maxX < 0)
			maxX /= xMult;
		else
			maxX *= xMult;

		if (minY < 0)
			minY *= yMult;
		else
			minY /= yMult;

		if (maxY < 0)
			maxY /= yMult;
		else
			maxY *= yMult;

		if (minZ < 0)
			minZ *= zMult;
		else
			minZ /= zMult;

		if (maxZ < 0)
			maxZ /= zMult;
		else
			maxZ *= zMult;

		const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

		return lightProjection * lightView;
	}
}

void Renderer::Init()
{
	RenderCommand::Init();

	s_UBO_scene_data = UniformBuffer::Create(sizeof(float) * 20, 0, UniformBuffer::Type::Dynamic);

	FramebufferSpecification specs;
	specs.width = GraphicsConfig::GetShadowBufferWidth();
	specs.height = GraphicsConfig::GetShadowBufferHeight();
	specs.samples = GraphicsConfig::GetShadowBufferSamples();
	specs.attachments = FramebufferAttachmentSpecification({ FramebufferTextureSpecification(FramebufferTextureFormat::Depth) });

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
	const auto& VP = camera.GetViewProjectionMatrix();
	const auto& pos = camera.GetPosition();
	s_UBO_scene_data->SetData(&VP, sizeof(float) * 16, 0);
	s_UBO_scene_data->SetData(&pos, sizeof(float) * 4, 64);
}

void Renderer::BeginScene(const PerspectiveCamera& camera)
{
	const auto& PV = camera.GetProjectionViewMatrix();
	const auto& pos = camera.GetPosition();

	s_UBO_scene_data->SetData(&PV, sizeof(float) * 16, 0);
	s_UBO_scene_data->SetData(&pos, sizeof(float) * 4, 64);

	// Right now we are choping zFar of frustum for better shadows
	// Cascaded Shadow Mapping would be nice to implement in future
	s_light_projection_view = CalculateLightMatrix(camera.GetProjectionMatrix(0.2f), camera.GetViewMatrix());
	s_view_frustum = camera.GetViewFrustum();
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
	shader->SetMat4(ShaderConstants::ShadowLightSpace, s_light_projection_view);

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

void Renderer::UpdateShadowMap(const Shader* shader)
{
	shader->Bind();
	Texture2D::BindTexture(s_framebuffer_shadows->GetDepthAttachmentID(), 3);
	shader->SetInt(ShaderConstants::ShadowMap, 3);
	shader->Unbind();
}
