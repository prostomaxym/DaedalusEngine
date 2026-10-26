#include "dlpch.h"

#include "SSAOPass.h"
#include "Renderer/Primitives/FrameBuffer.h"

#include "Renderer/Resources/ResourceManager.h"
#include "Config/GraphicsConfig.h"
#include "Renderer/API/RenderConstants.h"
#include "Renderer/API/Renderer.h"
#include "Renderer/API/RenderCommand.h"

using namespace Daedalus;

void SSAOPass::Render(FrameContext& context)
{
	if (!GraphicsConfig::IsSSBOEnabled())
		return;


	// ----------------------------------------- Main SSAO Pass --------------------------------------------- //
	const auto ssao_buffer = m_ssao->GetSSAOFramebuffer();
	ssao_buffer->Bind();
	RenderCommand::Clear(RendererAPI::ClearMode::ColorBuffer);

	const auto ssao_shader = ResourceManager::GetShader(ShaderConstants::SSAOShader);
	ssao_shader->Bind();
	Texture2D::BindTexture(context.GetTexture(PassTexture::Position), 0);
	Texture2D::BindTexture(context.GetTexture(PassTexture::Normal), 1);
	Texture2D::BindTexture(m_ssao->GetNoiseTexture()->GetRendererID(), 2);
	ssao_shader->SetInt(ShaderConstants::GBufferPos, 0);
	ssao_shader->SetInt(ShaderConstants::GBufferNorm, 1);
	ssao_shader->SetInt(ShaderConstants::SSAOBufferNoise, 2);
	ssao_shader->SetMat4(ShaderConstants::SSAOProjection, context.scene_proj);
	ssao_shader->SetMat4(ShaderConstants::SSAOView, context.scene_view);

	Renderer::DrawUnitQuad();
	ssao_shader->Unbind();
	ssao_buffer->Unbind();

	// ----------------------------------------- Blur SSAO Pass --------------------------------------------- //
	const auto blur_buffer = m_ssao->GetBlurFramebuffer();
	blur_buffer->Bind();
	RenderCommand::Clear(RendererAPI::ClearMode::ColorBuffer);

	const auto blue_shader = ResourceManager::GetShader(ShaderConstants::BlurShader);
	blue_shader->Bind();
	Texture2D::BindTexture(m_ssao->GetSSAOFramebuffer()->GetColorAttachmentRendererID(0), 0);
	blue_shader->SetInt(ShaderConstants::SSAOBlurBufferNoise, 0);

	Renderer::DrawUnitQuad();
	blue_shader->Unbind();
	blur_buffer->Unbind();

	context.SetTexture(PassTexture::SSAO, m_ssao->GetBlurFramebuffer()->GetColorAttachmentRendererID(0));
}

void SSAOPass::CreateSSAOBuffers(int width, int height)
{
	m_ssao = std::make_unique<SSAO>(width, height, GraphicsConfig::GetSSAOKernelSize(), 4, 4);
	m_ssao->CreateUBO(width, height);
}