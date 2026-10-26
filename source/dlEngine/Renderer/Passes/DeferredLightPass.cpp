#include "dlpch.h"

#include "DeferredLightPass.h"
#include "Renderer/Resources/Texture.h"
#include "Renderer/API/Renderer.h"
#include "Renderer/API/RenderCommand.h"
#include "Renderer/API/RenderConstants.h"
#include "Config/GraphicsConfig.h"
#include "Renderer/Resources/ResourceManager.h"

using namespace Daedalus;

void DeferredLightPass::Render(FrameContext& context)
{
	RenderCommand::SetViewport(0, 0, context.viewport_width, context.viewport_height);
	RenderCommand::Clear(RendererAPI::ClearMode::ColorBuffer | RendererAPI::ClearMode::DepthBuffer);
	const auto light_shader = ResourceManager::GetShader(ShaderConstants::DeferredLightShader);
	light_shader->Bind();

	Texture2D::BindTexture(context.GetTexture(PassTexture::Position), 0);
	light_shader->SetInt(ShaderConstants::GBufferPos, 0);
	Texture2D::BindTexture(context.GetTexture(PassTexture::Normal), 1);
	light_shader->SetInt(ShaderConstants::GBufferNorm, 1);
	Texture2D::BindTexture(context.GetTexture(PassTexture::Ambient), 2);
	light_shader->SetInt(ShaderConstants::GBufferAmbient, 2);
	Texture2D::BindTexture(context.GetTexture(PassTexture::Specular), 3);
	light_shader->SetInt(ShaderConstants::GBufferSpec, 3);
	Texture2D::BindTexture(context.GetTexture(PassTexture::Albedo), 4);
	light_shader->SetInt(ShaderConstants::GBufferAlbedo, 4);
	Texture2D::BindTexture(context.GetTexture(PassTexture::Shininess), 5);
	light_shader->SetInt(ShaderConstants::GBufferShininess, 5);

	if (GraphicsConfig::IsSSBOEnabled())
	{
		Texture2D::BindTexture(context.GetTexture(PassTexture::SSAO), 6);
		light_shader->SetInt(ShaderConstants::SSAOFinalBuffer, 6);
	}

	Texture2D::BindTexture(context.GetTexture(PassTexture::ShadowMap), 7);
	light_shader->SetInt(ShaderConstants::ShadowMaps, 7);

	Renderer::DrawUnitQuad();
	light_shader->Unbind();
}
