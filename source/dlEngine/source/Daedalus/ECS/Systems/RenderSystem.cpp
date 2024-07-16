#include "dlpch.h"
#include "RenderSystem.h"

#include "Daedalus/ECS/Components.h"
#include "Daedalus/ECS/Entity.h"
#include "Daedalus/ECS/Scene.h"
#include "Daedalus/Config/GraphicsConfig.h"
#include "Daedalus/Renderer/API/Renderer.h"
#include "Daedalus/Renderer/API/RenderConstants.h"

using namespace Daedalus;

void RenderSystem::OnStartRuntime() const
{
	Renderer::UpdateNumberOfShadowMap(CountShadowCasters());
}

void RenderSystem::OnUpdateRuntime(DeltaTime dt) const
{
	RenderCommand::SetClearColor({ 0.0f, 0.0f, 0.0f, 1.0 });
	Renderer::BeginScene(m_camera);

	UpdateLighting();
	DoShadowPass();
	DoDeferredGeometryPass();
	DoDeferredLightPass();

	Renderer::EndScene();
}

void RenderSystem::DoShadowPass() const
{
	if (CountShadowCasters() <= 0)
		return;

	const auto shadow_fb = Renderer::GetShadowFramebuffer();
	shadow_fb->Bind();
	RenderCommand::Clear(RendererAPI::ClearMode::DepthBuffer);
	RenderCommand::SetViewport(0, 0, GraphicsConfig::GetShadowBufferWidth(), GraphicsConfig::GetShadowBufferHeight());

	const auto shadow_shader = Renderer::GetShaderLibrary()->Get(ShaderConstants::ShadowShader);
	shadow_shader->Bind();
	const auto models_view = m_registry.view<RenderableObjectComponent>();
	for (const auto e : models_view)
	{
		Entity entity = { e, m_scene };

		const auto& model_component = entity.GetComponent<RenderableObjectComponent>();
		const auto& transform_component = entity.GetComponent<TransformComponent>().GetTransform();

		Renderer::SubmitForShadowBuffer(shadow_shader.get(), &model_component.model, transform_component);
	}

	shadow_shader->Unbind();
	shadow_fb->Unbind();
}

void RenderSystem::DoDeferredGeometryPass() const
{
	const auto gbuffer_shader = Renderer::GetShaderLibrary()->Get(ShaderConstants::DeferredGShader);
	
	const auto gbuffer = Renderer::GetGBuffer();
	gbuffer->Bind();
	RenderCommand::SetViewport(0, 0, m_viewport_width, m_viewport_height);
	RenderCommand::Clear(RendererAPI::ClearMode::ColorBuffer | RendererAPI::ClearMode::DepthBuffer);

	// const auto cubemap_shader = Renderer::GetShaderLibrary()->Get(ShaderConstants::CubemapShader);
	// cubemap_shader->Bind();
	// const auto cubemap_view = m_registry.view<SkyboxComponent>();
	// for (const auto e : cubemap_view)
	// {
	// 	Entity entity = { e, m_scene };
	//
	// 	const auto& cubemap_component = entity.GetComponent<SkyboxComponent>();
	// 	Renderer::Submit(cubemap_shader.get(), &cubemap_component.cubemap,
	// 		m_camera->GetProjectionViewMatrixWithoutTranslation(cubemap_component.rotation_angle));
	// }
	// cubemap_shader->Unbind();

	gbuffer_shader->Bind();
	const auto models_view = m_registry.view<RenderableObjectComponent>();
	for (const auto e : models_view)
	{
		Entity entity = { e, m_scene };

		const auto& model_component = entity.GetComponent<RenderableObjectComponent>();
		const auto& transform_component = entity.GetComponent<TransformComponent>().GetTransform();

		Renderer::Submit(gbuffer_shader.get(), &model_component.model, transform_component);
	}
	gbuffer_shader->Unbind();
	gbuffer->Unbind();
}

void RenderSystem::DoDeferredLightPass() const
{
	RenderCommand::SetViewport(0, 0, m_viewport_width, m_viewport_height);
	RenderCommand::Clear(RendererAPI::ClearMode::ColorBuffer | RendererAPI::ClearMode::DepthBuffer);
	const auto light_shader = Renderer::GetShaderLibrary()->Get(ShaderConstants::DeferredLightShader);
	light_shader->Bind();

	Renderer::BindGBufferTextures(light_shader.get(), 0);
	Renderer::BindShadowMap(light_shader.get(), 4);

	Renderer::DrawUnitQuad();
	light_shader->Unbind();
}

BoundingSphere RenderSystem::CalculateSceneBoundingSphere() const
{
	const auto models_view = m_registry.view<RenderableObjectComponent>();

	std::vector<BoundingSphere> spheres;
	spheres.reserve(models_view.size());

	for (const auto e : models_view)
	{
		Entity entity = { e, m_scene };

		const auto& model_component = entity.GetComponent<RenderableObjectComponent>();
		spheres.push_back(model_component.model.GetBoundingSphere());
	}

	return BoundingSphere::CalculateCommonBoundingSphere(spheres);
}

AABB RenderSystem::CalculateSceneBoundingAABB() const
{
	const auto models_view = m_registry.view<RenderableObjectComponent>();

	std::vector<AABB> aabbs;
	aabbs.reserve(models_view.size());

	for (const auto e : models_view)
	{
		Entity entity = { e, m_scene };

		const auto& model_component = entity.GetComponent<RenderableObjectComponent>();
		aabbs.push_back(model_component.model.GetBoundingAABB());
	}

	return AABB::CalculateCommonBoundingAABB(aabbs);
}

void RenderSystem::UpdateLighting() const
{
	std::vector<LightSSBO> light_SSBOs;
	std::vector<glm::mat4> light_proj_view;

	const auto proj = m_camera->GetProjectionMatrix();
	const auto view = m_camera->GetViewMatrix();

	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	for (const auto e : dir_view)
	{
		Entity entity = { e, m_scene };
		auto& light_component = entity.GetComponent<DirectionalLightComponent>();
		auto& light = light_component.light;

		if (light.CastShadow())
		{
			light.SetShadowMapIndex(light_proj_view.size());
			const auto cascades = light.CalculateCascadesProjView(proj, view);
			light_proj_view.insert(light_proj_view.end(), cascades.begin(), cascades.end());
		}
			
		light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	const auto point_view = m_registry.view<PointLightComponent>();
	for (const auto e : point_view)
	{
		Entity entity = { e, m_scene };
		auto& light_component = entity.GetComponent<PointLightComponent>();
		auto& light = light_component.light;

		if (light.CastShadow())
		{
			light.SetShadowMapIndex(light_proj_view.size());
			const auto cubemap = light.CalculateShadowCubemapProjView(proj);
			light_proj_view.insert(light_proj_view.end(), cubemap.begin(), cubemap.end());
		}
		light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	const auto spot_view = m_registry.view<SpotLightComponent>();
	for (const auto e : spot_view)
	{
		Entity entity = { e, m_scene };
		auto& light_component = entity.GetComponent<SpotLightComponent>();
		auto& light = light_component.light;

		if (light.CastShadow())
		{
			light.SetShadowMapIndex(light_proj_view.size());
			const auto cascades = light.CalculateCascadesProjView(proj, view);
				light_proj_view.insert(light_proj_view.end(), cascades.begin(), cascades.end());
		}

		light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	Renderer::UpdateLightSpaceMatricesSSBO(light_proj_view);
	Renderer::UpdateLightSSBO(light_SSBOs);
}

int RenderSystem::CountShadowCasters() const
{
	int counter = 0;
	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	for (const auto e : dir_view)
	{
		Entity entity = { e, m_scene };
		const auto& light_component = entity.GetComponent<DirectionalLightComponent>();

		if (light_component.light.CastShadow())
			counter += light_component.light.GetShadowNumberOfCascades();
	}

	const auto point_view = m_registry.view<PointLightComponent>();
	for (const auto e : point_view)
	{
		Entity entity = { e, m_scene };
		const auto& light_component = entity.GetComponent<PointLightComponent>();

		if (light_component.light.CastShadow())
			counter += 6;

	}
	const auto spot_view = m_registry.view<SpotLightComponent>();
	for (const auto e : spot_view)
	{
		Entity entity = { e, m_scene };
		const auto& light_component = entity.GetComponent<SpotLightComponent>();

		if (light_component.light.CastShadow())
			counter += light_component.light.GetShadowNumberOfCascades();
	}

	return counter;
}