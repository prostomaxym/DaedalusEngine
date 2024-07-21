#include "dlpch.h"

#include "SSAOPass.h"

#include "Daedalus/Config/GraphicsConfig.h"
#include "Daedalus/Renderer/API/RenderConstants.h"
#include "Daedalus/Renderer/API/Renderer.h"

using namespace Daedalus;

SSAOPass::PassOut SSAOPass::Render(const PassIn& data) const
{
	// ----------------------------------------- Main SSAO Pass --------------------------------------------- //
	const auto ssao_buffer = m_ssao->GetSSAOFramebuffer();
	ssao_buffer->Bind();
	RenderCommand::Clear(RendererAPI::ClearMode::ColorBuffer);

	const auto ssao_shader = Renderer::GetShaderLibrary()->Get(ShaderConstants::SSAOShader);
	ssao_shader->Bind();
	Texture2D::BindTexture(data.position_texture, 0);
	Texture2D::BindTexture(data.normal_texture, 1);
	Texture2D::BindTexture(m_ssao->GetNoiseTexture()->GetRendererID(), 2);
	ssao_shader->SetInt(ShaderConstants::GBufferPos, 0);
	ssao_shader->SetInt(ShaderConstants::GBufferNorm, 1);
	ssao_shader->SetInt(ShaderConstants::SSAOBufferNoise, 2);
	ssao_shader->SetMat4(ShaderConstants::SSAOProjection, data.proj);
	ssao_shader->SetMat4(ShaderConstants::SSAOView, data.view);

	Renderer::DrawUnitQuad();
	ssao_shader->Unbind();
	ssao_buffer->Unbind();

	// ----------------------------------------- Blur SSAO Pass --------------------------------------------- //
	const auto blur_buffer = m_ssao->GetBlurFramebuffer();
	blur_buffer->Bind();
	RenderCommand::Clear(RendererAPI::ClearMode::ColorBuffer);

	const auto blue_shader = Renderer::GetShaderLibrary()->Get(ShaderConstants::BlurShader);
	blue_shader->Bind();
	Texture2D::BindTexture(m_ssao->GetSSAOFramebuffer()->GetColorAttachmentRendererID(0), 0);
	blue_shader->SetInt(ShaderConstants::SSAOBlurBufferNoise, 0);

	Renderer::DrawUnitQuad();
	blue_shader->Unbind();
	blur_buffer->Unbind();

	return PassOut{ m_ssao->GetBlurFramebuffer()->GetColorAttachmentRendererID(0) };
}

void SSAOPass::CreateSSAOBuffers(int width, int height)
{
	m_ssao = std::make_unique<SSAO>(width, height, GraphicsConfig::GetSSAOKernelSize(), 4, 4);
	m_ssao->CreateUBO(width, height);
}