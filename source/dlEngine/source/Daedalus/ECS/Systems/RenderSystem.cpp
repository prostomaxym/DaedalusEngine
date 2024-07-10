#include "dlpch.h"
#include "RenderSystem.h"

#include "Daedalus/ECS/Components.h"
#include "Daedalus/ECS/Entity.h"
#include "Daedalus/ECS/Scene.h"
#include "Daedalus/Config/GraphicsConfig.h"
#include "Daedalus/Renderer/API/Renderer.h"
#include "Daedalus/Renderer/API/RenderConstants.h"

using namespace Daedalus;

void RenderSystem::OnStartRuntime()
{
	UpdateStaticLighting();
	Renderer::UpdateNumberOfShadowCasters(CountShadowCasters());
}

void RenderSystem::OnUpdateRuntime(DeltaTime dt) const
{
	RenderCommand::SetClearColor({ 1.0f, 1.0f, 1.0f, 1.0 });
	Renderer::BeginScene(m_camera);

	UpdateDynamicLighting();
	DoLightPass();
	DoColorPass();

	Renderer::EndScene();
}

void RenderSystem::DoLightPass() const
{
	if (CountShadowCasters() <= 0)
		return;

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

void RenderSystem::DoColorPass() const
{
	const auto standard_shader = Renderer::s_shader_library->Get(ShaderConstants::StandardShader);
	Renderer::BindShadowMap(standard_shader.get());
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

int RenderSystem::CountShadowCasters() const
{
	int counter = 0;
	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	for (const auto e : dir_view)
	{
		Entity entity = { e, m_scene };
		const auto& light_component = entity.GetComponent<DirectionalLightComponent>();

		if (light_component.light.CastShadow())
			counter++;
	}

	const auto spot_view = m_registry.view<SpotLightComponent>();
	for (const auto e : spot_view)
	{
		Entity entity = { e, m_scene };
		const auto& light_component = entity.GetComponent<SpotLightComponent>();

		if (light_component.light.CastShadow())
			counter++;
	}

	return counter;
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

void RenderSystem::UpdateStaticLighting()
{
	const auto scene_sphere = CalculateSceneBoundingSphere();
    m_static_light_proj_view = std::vector<glm::mat4>();
	m_static_light_view = std::vector<glm::mat4>();
	std::vector<LightSSBO> light_SSBOs;
	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	for (const auto e : dir_view)
	{
		Entity entity = { e, m_scene };
		auto& light_component = entity.GetComponent<DirectionalLightComponent>();
		auto& light = light_component.light;

		if (light_component.is_dynamic)
			continue;

		if (light.CastShadow())
		{
			light.SetShadowMapIndex(m_static_light_proj_view.size());
			light.SetShadowNumberOfCascades(1);
			m_static_light_proj_view.push_back(light.CalculateProjViewForSphere(scene_sphere));
			m_static_light_view.emplace_back(glm::mat4());
		}

		light_SSBOs.emplace_back(light.GetShaderSSBO());
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
		auto& light_component = entity.GetComponent<SpotLightComponent>();
		auto& light = light_component.light;

		if (light_component.is_dynamic)
			continue;

		if (light.CastShadow())
		{
			light.SetShadowMapIndex(m_static_light_proj_view.size());
			light.SetShadowNumberOfCascades(1);
			m_static_light_proj_view.push_back(light.CalculateProjViewForFrustum(m_camera->GetProjectionMatrix(), m_camera->GetViewMatrix()));
			m_static_light_view.emplace_back(light.CalculateView());
		}

		light_SSBOs.emplace_back(light.GetShaderSSBO());
	}

	Renderer::UpdateStaticLightSSBO(light_SSBOs);
}

void RenderSystem::UpdateDynamicLighting() const
{
	std::vector<LightSSBO> light_SSBOs;
	std::vector<glm::mat4> light_proj_view = m_static_light_proj_view;
	std::vector<glm::mat4> light_view = m_static_light_view;
	const auto perc = RendererConstants::CascadePercents;

	const auto dir_view = m_registry.view<DirectionalLightComponent>();
	for (const auto e : dir_view)
	{
		Entity entity = { e, m_scene };
		auto& light_component = entity.GetComponent<DirectionalLightComponent>();
		auto& light = light_component.light;

		if (!light_component.is_dynamic)
			continue;

		if (light.CastShadow())
		{
			light.SetShadowMapIndex(light_proj_view.size());
			light.SetShadowNumberOfCascades(RendererConstants::NumberOfShadowCascades);
			light_proj_view.push_back(light.CalculateProjViewForFrustum(m_camera->GetProjectionMatrix(0.f, perc.x), m_camera->GetViewMatrix()));
			light_proj_view.push_back(light.CalculateProjViewForFrustum(m_camera->GetProjectionMatrix(perc.x, perc.y), m_camera->GetViewMatrix()));
			light_proj_view.push_back(light.CalculateProjViewForFrustum(m_camera->GetProjectionMatrix(perc.y, perc.z), m_camera->GetViewMatrix()));
			light_view.emplace_back(m_camera->GetViewMatrix());
			light_view.emplace_back(m_camera->GetViewMatrix());
			light_view.emplace_back(m_camera->GetViewMatrix());
		}
			
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
		auto& light_component = entity.GetComponent<SpotLightComponent>();
		auto& light = light_component.light;

		if (!light_component.is_dynamic)
			continue;

		if (light.CastShadow())
		{
			light.SetShadowMapIndex(light_proj_view.size());
			light.SetShadowNumberOfCascades(RendererConstants::NumberOfShadowCascades);
			light_proj_view.push_back(light.CalculateProjViewForFrustum(m_camera->GetProjectionMatrix(0.f, perc.x), m_camera->GetViewMatrix()));
			light_proj_view.push_back(light.CalculateProjViewForFrustum(m_camera->GetProjectionMatrix(perc.x, perc.y), m_camera->GetViewMatrix()));
			light_proj_view.push_back(light.CalculateProjViewForFrustum(m_camera->GetProjectionMatrix(perc.y, perc.z), m_camera->GetViewMatrix()));
			light_view.emplace_back(light.CalculateView());
			light_view.emplace_back(light.CalculateView());
			light_view.emplace_back(light.CalculateView());
		}

		light_SSBOs.emplace_back(light_component.light.GetShaderSSBO());
	}

	Renderer::UpdateLightSpaceMatricesSSBO(light_proj_view, light_view);
	Renderer::UpdateDynamicLightSSBO(light_SSBOs);
}