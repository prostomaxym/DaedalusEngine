#include "dlpch.h"
#include "RenderSystem.h"

#include "Daedalus/ECS/Components.h"
#include "Daedalus/ECS/Entity.h"
#include "Daedalus/ECS/Scene.h"
#include "Daedalus/Renderer/API/Renderer.h"

using namespace Daedalus;

void RenderSystem::OnUpdateRuntime(DeltaTime dt)
{
		RenderCommand::SetClearColor({ 0.2f, 0.2f, 0.2f, 1.f });
		Renderer::BeginScene(*m_camera);

		UpdateDynamicLighting();

		const auto shadow_fb = Renderer::GetShadowFramebuffer();
		shadow_fb->Bind();
		RenderCommand::Clear(RendererAPI::ClearMode::DepthBuffer);
		RenderCommand::SetViewport(0, 0, 2048, 2048);

		auto shadow_shader = Renderer::s_shader_library->Get("Shadow");

		const auto models_view = m_registry.view<RenderableObjectComponent>();
		for (auto e : models_view)
		{
			Entity entity = { e, m_scene };

			const auto& model_component = entity.GetComponent<RenderableObjectComponent>();
			Renderer::SubmitForShadowBuffer(shadow_shader.get(), &model_component.model, entity.GetComponent<TransformComponent>().GetTransform());
		}
		shadow_fb->Unbind();


		Renderer::UpdateShadowMap();
		RenderCommand::SetViewport(0, 0, m_viewport_width, m_viewport_height);
		RenderCommand::Clear(RendererAPI::ClearMode::ColorBuffer | RendererAPI::ClearMode::DepthBuffer);

		for (auto e : models_view)
		{
			Entity entity = { e, m_scene };

			const auto& model_component = entity.GetComponent<RenderableObjectComponent>();
			Renderer::Submit(model_component.shader.get(), &model_component.model, entity.GetComponent<TransformComponent>().GetTransform());
		}

		const auto cubemap_view = m_registry.view<CubemapComponent>();
		for (auto e : cubemap_view)
		{
			Entity entity = { e, m_scene };

			const auto& cubemap_component = entity.GetComponent<CubemapComponent>();
			Renderer::Submit(cubemap_component.shader.get(), &cubemap_component.cubemap, m_camera->GetProjectionViewMatrixWithoutTranslation(cubemap_component.rotation_angle));
		}

		Renderer::EndScene();
}

void RenderSystem::UpdateStaticLighting()
{
	std::vector<LightSSBO> light_SSBOs;
	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	for (auto e : dir_view)
	{
		Entity entity = { e, m_scene };
		const auto& light_component = entity.GetComponent<DirectionalLightComponent>();

		if (!light_component.is_dynamic)
			light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	const auto point_view = m_registry.view<PointLightComponent>();
	for (auto e : point_view)
	{
		Entity entity = { e, m_scene };

		const auto& light_component = entity.GetComponent<PointLightComponent>();

		if (!light_component.is_dynamic)
			light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	const auto spot_view = m_registry.view<SpotLightComponent>();
	for (auto e : spot_view)
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
	for (auto e : dir_view)
	{
		Entity entity = { e, m_scene };
		const auto& light_component = entity.GetComponent<DirectionalLightComponent>();

		if (light_component.is_dynamic)
			light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	const auto point_view = m_registry.view<PointLightComponent>();
	for (auto e : point_view)
	{
		Entity entity = { e, m_scene };
		const auto& light_component = entity.GetComponent<PointLightComponent>();

		if (light_component.is_dynamic)
			light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	const auto spot_view = m_registry.view<SpotLightComponent>();
	for (auto e : spot_view)
	{
		Entity entity = { e, m_scene };
		const auto& light_component = entity.GetComponent<SpotLightComponent>();

		if (light_component.is_dynamic)
			light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	Renderer::UpdateDynamicLightSSBO(light_SSBOs);
}
