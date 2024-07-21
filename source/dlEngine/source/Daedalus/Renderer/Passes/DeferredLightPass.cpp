#include "dlpch.h"

#include "DeferredLightPass.h"
#include "Daedalus/Renderer/API/Renderer.h"
#include "Daedalus/Renderer/API/RenderConstants.h"
#include "Daedalus/Config/GraphicsConfig.h"

using namespace Daedalus;

void DeferredLightPass::Render(const PassIn& data) const
{
	RenderCommand::SetViewport(0, 0, data.width, data.height);
	RenderCommand::Clear(RendererAPI::ClearMode::ColorBuffer | RendererAPI::ClearMode::DepthBuffer);
	const auto light_shader = Renderer::GetShaderLibrary()->Get(ShaderConstants::DeferredLightShader);
	light_shader->Bind();

	Texture2D::BindTexture(data.pos_texture, 0);
	light_shader->SetInt(ShaderConstants::GBufferPos, 0);
	Texture2D::BindTexture(data.norm_texture, 1);
	light_shader->SetInt(ShaderConstants::GBufferNorm, 1);
	Texture2D::BindTexture(data.ambient_texture, 2);
	light_shader->SetInt(ShaderConstants::GBufferAmbient, 2);
	Texture2D::BindTexture(data.spec_texture, 3);
	light_shader->SetInt(ShaderConstants::GBufferSpec, 3);
	Texture2D::BindTexture(data.albedo_texture, 4);
	light_shader->SetInt(ShaderConstants::GBufferAlbedo, 4);
	Texture2D::BindTexture(data.shininess_texture, 5);
	light_shader->SetInt(ShaderConstants::GBufferShininess, 5);

	if (GraphicsConfig::IsSSBOEnabled())
	{
		Texture2D::BindTexture(data.ssao_texture, 6);
		light_shader->SetInt(ShaderConstants::SSAOFinalBuffer, 6);
	}

	Texture2D::BindTexture(data.shadowmap_texture, 7);
	light_shader->SetInt(ShaderConstants::ShadowMaps, 7);

	Renderer::DrawUnitQuad();
	light_shader->Unbind();
}

DeferredLightPass::PassIn::PassIn(const DeferredGeometryPass::PassOut& geom, uint32_t shadowmap, int w, int h)
{
	width = w;
	height = h;

	pos_texture = geom.pos_texture;
	norm_texture = geom.norm_texture;
	ambient_texture = geom.ambient_texture;
	spec_texture = geom.spec_texture;
	albedo_texture = geom.albedo_texture;
	shininess_texture = geom.shininess_texture;
	shadowmap_texture = shadowmap;
}
