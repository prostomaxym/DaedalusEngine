#include "dlpch.h"
#include "ShadowPass.h"

#include "Core/ResourceManager.h"
#include "Config/GraphicsConfig.h"
#include "Renderer/API/RenderCommand.h"
#include "Renderer/API/Renderer.h"
#include "Renderer/API/RenderConstants.h"

using namespace Daedalus;

void ShadowPass::SetNumberOfShadowMaps(int number)
{
	// Limiting buffer resizing to minimize allocations/deallocation.
	// Do not recreate framebuffer unless size is twice smaller
	if (number > m_number_of_shadowmaps || number <= m_number_of_shadowmaps / 2)
	{
		FramebufferSpecification specs;
		specs.width = GraphicsConfig::GetShadowBufferWidth();
		specs.height = GraphicsConfig::GetShadowBufferHeight();
		specs.samples = GraphicsConfig::GetShadowBufferSamples();
		specs.attachments = FramebufferAttachmentSpecification({ FramebufferTextureSpecification(Texture::ColorFormat::Depth) });
		specs.layers = number;

		m_framebuffer = Framebuffer::Create(specs);
	}

	m_number_of_shadowmaps = number;
}

ShadowPass::PassOut ShadowPass::Render(const PassIn& data)const
{
	if (m_number_of_shadowmaps <= 0)
		return PassOut();

	m_framebuffer->Bind();
	RenderCommand::Clear(RendererAPI::ClearMode::DepthBuffer);
	RenderCommand::SetViewport(0, 0, GraphicsConfig::GetShadowBufferWidth(), GraphicsConfig::GetShadowBufferHeight());
	 
	const auto shadow_shader = ResourceManager::GetShader(ShaderConstants::ShadowShader);
	shadow_shader->Bind();

	for (const auto& model_data : data.frame_models)
	{
		shadow_shader->SetMat4(ShaderConstants::ShadowModel, model_data.second);
		const auto& meshes = model_data.first->GetMeshes();

		for (const auto& mesh : meshes)
		{
			const auto vertex_array = mesh->GetVertexArray();
			RenderCommand::DrawIndexed(vertex_array.get());
		}
	}

	shadow_shader->Unbind();
	m_framebuffer->Unbind();

	return PassOut{ m_framebuffer->GetDepthAttachmentID() };
}