#include "dlpch.h"

#include "DeferredGeometryPass.h"
#include "Renderer/API/Renderer.h"
#include "Renderer/API/RenderConstants.h"
#include "Config/GraphicsConfig.h"

using namespace Daedalus;

DeferredGeometryPass::PassOut DeferredGeometryPass::Render(const PassIn& data) const
{
	const auto gbuffer_shader = Renderer::GetShaderLibrary()->Get(ShaderConstants::DeferredGShader);

	m_gbuffer->Bind();
	RenderCommand::SetViewport(0, 0, data.viewport_width, data.viewport_height);
	RenderCommand::Clear(RendererAPI::ClearMode::ColorBuffer | RendererAPI::ClearMode::DepthBuffer);

	gbuffer_shader->Bind();

	for (const auto& model_data : data.frame_models)
	{
		gbuffer_shader->SetMat4(ShaderConstants::SceneModel, model_data.second);

		const auto& meshes = model_data.first->GetMeshes();
		const auto& materials = model_data.first->GetMaterials();

		for (const auto& mesh : meshes)
		{
			if (!mesh->IsVisible(data.view_frustum, model_data.second))
				continue;

			const auto& material = materials[mesh->GetMaterialIndex()];

			gbuffer_shader->SetFloat3(ShaderConstants::MaterialKAmbient, material.GetAmbientK());
			gbuffer_shader->SetFloat3(ShaderConstants::MaterialKDiffuse, material.GetDiffuseK());
			gbuffer_shader->SetFloat3(ShaderConstants::MaterialKSpecular, material.GetSpecularK());
			gbuffer_shader->SetFloat(ShaderConstants::MaterialShininess, material.GetShininess());

			if (const auto& diffuse_map = material.GetDiffuseMap(); diffuse_map)
			{
				diffuse_map->Bind(0);
				gbuffer_shader->SetInt(ShaderConstants::ConfigDiffuseMapUsed, 1);
				gbuffer_shader->SetInt(ShaderConstants::MaterialTexDiffuse, 0);
			}
			else
			{
				RenderCommand::UnbindTextureSlot(0);
				gbuffer_shader->SetInt(ShaderConstants::ConfigDiffuseMapUsed, 0);
			}

			if (const auto& specular_map = material.GetSpecularMap(); specular_map)
			{
				specular_map->Bind(1);
				gbuffer_shader->SetInt(ShaderConstants::ConfigSpecularMapUsed, 1);
				gbuffer_shader->SetInt(ShaderConstants::MaterialTexSpecular, 1);
			}
			else
			{
				RenderCommand::UnbindTextureSlot(1);
				gbuffer_shader->SetInt(ShaderConstants::ConfigSpecularMapUsed, 0);
			}

			if (const auto& normal_map = material.GetNormalMap(); normal_map)
			{
				normal_map->Bind(2);
				gbuffer_shader->SetInt(ShaderConstants::ConfigNormalMapUsed, 1);
				gbuffer_shader->SetInt(ShaderConstants::MaterialTexNormal, 2);
			}
			else
			{
				RenderCommand::UnbindTextureSlot(2);
				gbuffer_shader->SetInt(ShaderConstants::ConfigNormalMapUsed, 0);
			}

			if (const auto& height_map = material.GetHeightMap(); height_map)
			{
				height_map->Bind(3);
				gbuffer_shader->SetInt(ShaderConstants::ConfigHeightMapUsed, 1);
				gbuffer_shader->SetInt(ShaderConstants::MaterialTexHeight, 3);
			}
			else
			{
				RenderCommand::UnbindTextureSlot(3);
				gbuffer_shader->SetInt(ShaderConstants::ConfigHeightMapUsed, 0);
			}

			const auto vertex_array = mesh->GetVertexArray();
			RenderCommand::DrawIndexed(vertex_array.get());
		}
	}

	gbuffer_shader->Unbind();
	m_gbuffer->Unbind();

	PassOut out;

	out.pos_texture = m_gbuffer->GetColorAttachmentRendererID(0);
	out.norm_texture = m_gbuffer->GetColorAttachmentRendererID(1);
	out.ambient_texture = m_gbuffer->GetColorAttachmentRendererID(2);
	out.spec_texture = m_gbuffer->GetColorAttachmentRendererID(3);
	out.albedo_texture = m_gbuffer->GetColorAttachmentRendererID(4);
	out.shininess_texture = m_gbuffer->GetColorAttachmentRendererID(5);

	return out;
}

void DeferredGeometryPass::CreateGBuffer(int width, int height)
{
	FramebufferSpecification gbuffer_specs;
	gbuffer_specs.width = width;
	gbuffer_specs.height = height;
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
	m_gbuffer = Framebuffer::Create(gbuffer_specs);
}
