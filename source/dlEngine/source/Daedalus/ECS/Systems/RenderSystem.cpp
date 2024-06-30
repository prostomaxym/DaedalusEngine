#include "dlpch.h"
#include "RenderSystem.h"

#include "Daedalus/ECS/Components.h"
#include "Daedalus/ECS/Entity.h"
#include "Daedalus/ECS/Scene.h"
#include "Daedalus/Config/GraphicsConfig.h"
#include "Daedalus/Renderer/API/Renderer.h"
#include "Daedalus/Renderer/API/RenderConstants.h"

using namespace Daedalus;

void RenderSystem::OnUpdateRuntime(DeltaTime dt)
{
	RenderCommand::SetClearColor({ 0.0f, 0.0f, 0.0f, 1.0 });
	Renderer::BeginScene(*m_camera);

	UpdateDynamicLighting();

	DoLightPass();
	DoColorPass();

	Renderer::EndScene();
}

void RenderSystem::DoLightPass()
{
	const auto shadow_fb = Renderer::GetShadowFramebuffer();
	shadow_fb->Bind();
	RenderCommand::Clear(RendererAPI::ClearMode::DepthBuffer);
	RenderCommand::SetViewport(0, 0, GraphicsConfig::GetShadowBufferWidth(), GraphicsConfig::GetShadowBufferHeight());

	const auto shadow_shader = Renderer::s_shader_library->Get(ShaderConstants::ShadowShader);

	const auto models_view = m_registry.view<RenderableObjectComponent>();
	for (const auto e : models_view)
	{
		Entity entity = { e, m_scene };

		const auto& model_component = entity.GetComponent<RenderableObjectComponent>();
		const auto& transform_component = entity.GetComponent<TransformComponent>().GetTransform();

		Renderer::SubmitForShadowBuffer(shadow_shader.get(), &model_component.model, transform_component);
	}
	shadow_fb->Unbind();
}

void RenderSystem::DoColorPass()
{
	const auto standard_shader = Renderer::s_shader_library->Get(ShaderConstants::StandardShader);
	Renderer::UpdateShadowMap(standard_shader.get());
	RenderCommand::SetViewport(0, 0, m_viewport_width, m_viewport_height);
	RenderCommand::Clear(RendererAPI::ClearMode::ColorBuffer | RendererAPI::ClearMode::DepthBuffer);

	const auto models_view = m_registry.view<RenderableObjectComponent>();
	for (const auto e : models_view)
	{
		Entity entity = { e, m_scene };

		const auto& model_component = entity.GetComponent<RenderableObjectComponent>();
		const auto& transform_component = entity.GetComponent<TransformComponent>().GetTransform();

		Renderer::Submit(model_component.shader.get(), &model_component.model, transform_component);
	}

	const auto cubemap_view = m_registry.view<CubemapComponent>();
	for (const auto e : cubemap_view)
	{
		Entity entity = { e, m_scene };

		const auto& cubemap_component = entity.GetComponent<CubemapComponent>();
		Renderer::Submit(cubemap_component.shader.get(), &cubemap_component.cubemap, m_camera->GetProjectionViewMatrixWithoutTranslation(cubemap_component.rotation_angle));
	}
}

void RenderSystem::UpdateStaticLighting()
{
	std::vector<LightSSBO> light_SSBOs;
	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	for (const auto e : dir_view)
	{
		Entity entity = { e, m_scene };
		const auto& light_component = entity.GetComponent<DirectionalLightComponent>();

		if (!light_component.is_dynamic)
			light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	const auto point_view = m_registry.view<PointLightComponent>();
	for (const auto e : point_view)
	{
		Entity entity = { e, m_scene };

		const auto& light_component = entity.GetComponent<PointLightComponent>();

		if (!light_component.is_dynamic)
			light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	const auto spot_view = m_registry.view<SpotLightComponent>();
	for (const auto e : spot_view)
	{
		Entity entity = { e, m_scene };
		const auto& light_component = entity.GetComponent<SpotLightComponent>();

		if (!light_component.is_dynamic)
			light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	Renderer::UpdateStaticLightSSBO(light_SSBOs);
}

void RenderSystem::UpdateDynamicLighting()
{
	std::vector<LightSSBO> light_SSBOs;
	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	for (const auto e : dir_view)
	{
		Entity entity = { e, m_scene };
		const auto& light_component = entity.GetComponent<DirectionalLightComponent>();

		if (light_component.is_dynamic)
			light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	const auto point_view = m_registry.view<PointLightComponent>();
	for (const auto e : point_view)
	{
		Entity entity = { e, m_scene };
		const auto& light_component = entity.GetComponent<PointLightComponent>();

		if (light_component.is_dynamic)
			light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	const auto spot_view = m_registry.view<SpotLightComponent>();
	for (const auto e : spot_view)
	{
		Entity entity = { e, m_scene };
		const auto& light_component = entity.GetComponent<SpotLightComponent>();

		if (light_component.is_dynamic)
			light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	Renderer::UpdateDynamicLightSSBO(light_SSBOs);
}
